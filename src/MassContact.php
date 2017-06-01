<?php

namespace Drupal\mass_contact;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\Mail\MailManagerInterface;
use Drupal\Core\Queue\QueueFactory;
use Drupal\Core\Session\AccountInterface;
use Drupal\mass_contact\Entity\MassContactMessageInterface;

/**
 * The Mass Contact helper service.
 */
class MassContact implements MassContactInterface {

  /**
   * Number of recipients to queue into a single queue worker at a time.
   *
   * If sending via BCC, this also controls the number of recipients in a single
   * email.
   */
  const MAX_QUEUE_RECIPIENTS = 50;

  /**
   * Defines the HTML modules supported.
   *
   * @var string[]
   */
  protected static $htmlEmailModules = [
    'mimemail',
    'swiftmailer',
  ];

  /**
   * The mass contact settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   */
  protected $config;

  /**
   * The module handler.
   *
   * @var \Drupal\Core\Extension\ModuleHandlerInterface
   */
  protected $moduleHandler;

  /**
   * The opt-out service.
   *
   * @var \Drupal\mass_contact\OptOutInterface
   */
  protected $optOut;

  /**
   * The message queueing queue.
   *
   * @var \Drupal\Core\Queue\QueueInterface
   */
  protected $processingQueue;

  /**
   * The message sending queue.
   *
   * @var \Drupal\Core\Queue\QueueInterface
   */
  protected $sendingQueue;

  /**
   * The recipient grouping plugin manager.
   *
   * @var \Drupal\Core\Mail\MailManagerInterface
   */
  protected $mail;

  /**
   * The entity type manager service.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The current user.
   *
   * @var \Drupal\Core\Session\AccountInterface
   */
  protected $currentUser;

  /**
   * Constructs the Mass Contact helper.
   *
   * @param \Drupal\Core\Extension\ModuleHandlerInterface $module_handler
   *   The module handler service.
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Queue\QueueFactory $queue
   *   The queue factory.
   * @param \Drupal\Core\Mail\MailManagerInterface $mail_manager
   *   The mail plugin manager.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager service.
   * @param \Drupal\mass_contact\OptOutInterface $opt_out
   *   The mass contact opt-out service.
   * @param \Drupal\Core\Session\AccountInterface $current_user
   *   The current user.
   */
  public function __construct(ModuleHandlerInterface $module_handler, ConfigFactoryInterface $config_factory, QueueFactory $queue, MailManagerInterface $mail_manager, EntityTypeManagerInterface $entity_type_manager, OptOutInterface $opt_out, AccountInterface $current_user) {
    $this->moduleHandler = $module_handler;
    $this->config = $config_factory->get('mass_contact.settings');
    $this->optOut = $opt_out;
    $this->processingQueue = $queue->get('mass_contact_queue_messages', TRUE);
    $this->sendingQueue = $queue->get('mass_contact_send_message', TRUE);
    $this->mail = $mail_manager;
    $this->entityTypeManager = $entity_type_manager;
    $this->currentUser = $current_user;

  }

  /**
   * {@inheritdoc}
   */
  public function htmlSupported() {
    foreach (static::$htmlEmailModules as $module) {
      if ($this->moduleHandler->moduleExists($module)) {
        return TRUE;
      }
    }
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function processMassContactMessage(MassContactMessageInterface $message, array $configuration = []) {
    $configuration += $this->getDefaultConfiguration();
    $data = [
      'message' => $message,
      'configuration' => $configuration,
    ];
    $this->processingQueue->createItem($data);
    if ($configuration['create_archive_copy']) {
      $message->save();
    }
  }

  /**
   * Set default config values.
   *
   * @return array
   *   The default configuration as defined in the mass_contact.settings config.
   */
  protected function getDefaultConfiguration() {
    $default = [
      'use_bcc' => $this->config->get('use_bcc'),
      'sender_name' => $this->config->get('default_sender_name'),
      'sender_mail' => $this->config->get('default_sender_email'),
      'create_archive_copy' => $this->config->get('create_archive_copy'),
      'send_me_copy_email_address' => '',
    ];
    return $default;
  }

  /**
   * {@inheritdoc}
   */
  public function queueRecipients(MassContactMessageInterface $message, array $configuration = []) {

    $data = [
      'message' => $message,
      'configuration' => $configuration,
    ];
    $all_recipients = $this->getRecipients($message->getCategories());
    $send_me_a_copy_email_address = $data['configuration']['send_me_copy_email_address'];
    if ($send_me_a_copy_email_address) {
      // Add the sender's email to the recipient list if 'Send yourself a copy'
      // option has been chosen AND the email is not already in the recipient
      // list.
      if (empty($all_recipients) || (!in_array($send_me_a_copy_email_address, array_column($all_recipients, 'email')))) {
        $all_recipients[] = [
          'email' => $data['configuration']['send_me_copy_email_address'],
          'langcode' => $this->entityTypeManager->getStorage('user')
            ->load($this->currentUser->id())->langcode->value,
        ];
      }
    }
    foreach ($this->getGroupedRecipients($all_recipients) as $recipients) {
      $data['recipients'] = $recipients;
      $this->sendingQueue->createItem($data);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function getGroupedRecipients(array $all_recipients) {
    $groupings = [];
    $recipients = [];
    foreach ($all_recipients as $recipient) {
      $recipients[] = $recipient;
      if (count($recipients) == static::MAX_QUEUE_RECIPIENTS) {
        // Send in batches.
        $groupings[] = $recipients;
        $recipients = [];
      }
    }

    // If there are any left, group those too.
    if (!empty($recipients)) {
      $groupings[] = $recipients;
    }
    return $groupings;
  }

  /**
   * {@inheritdoc}
   */
  public function getRecipients(array $categories) {
    $recipients = [];
    if (!empty($categories)) {
      foreach ($categories as $category) {
        foreach ($category->getRecipients() as $plugin_id => $config) {
          $grouping = $category->getGroupingCategories($plugin_id);
          $recipients += $grouping->getRecipients($config['categories']);
        }
      }

      // Filter out users that have opted out.
      $recipients = array_diff_key($recipients, $this->optOut->getOptOutAccounts($categories));

      // We should have a unique list of users at this point.  (Adding arrays
      // behaves that way).
      $user_storage = $this->entityTypeManager->getStorage('user');
      $recipients_emails = [];
      foreach ($recipients as $key => $value) {
        $user = $user_storage->load($key);
        $recipients_emails[$key] = ['email' => $user->getEmail(), 'langcode' => $user->langcode->value];
      }

      return $recipients_emails;
    }
  }

  /**
   * {@inheritdoc}
   */
  public function sendMessage(array $recipients, MassContactMessageInterface $message, array $configuration = []) {
    $params = [
      'subject' => $message->getSubject(),
      'body' => $message->getBody(),
      'format' => $message->getFormat(),
      'configuration' => $configuration,
      'headers' => [],
    ];

    // If utilizing BCC, one email is sent.
    if ($configuration['use_bcc']) {
      $params['headers']['Bcc'] = implode(',', array_unique(array_column($recipients, 'email')));
      $this->mail->mail('mass_contact', 'mass_contact', $configuration['sender_mail'], \Drupal::languageManager()->getDefaultLanguage()->getId(), $params);
    }
    else {
      foreach ($recipients as $recipient) {
        $this->mail->mail('mass_contact', 'mass_contact', $recipient['email'], $recipient['langcode'], $params);
      }
    }
  }

}
