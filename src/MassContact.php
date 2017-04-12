<?php

namespace Drupal\mass_contact;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\Mail\MailManagerInterface;
use Drupal\Core\Queue\QueueFactory;

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
   */
  public function __construct(ModuleHandlerInterface $module_handler, ConfigFactoryInterface $config_factory, QueueFactory $queue, MailManagerInterface $mail_manager, EntityTypeManagerInterface $entity_type_manager) {
    $this->moduleHandler = $module_handler;
    $this->config = $config_factory->get('mass_contact.settings');
    $this->processingQueue = $queue->get('mass_contact_queue_messages', TRUE);
    $this->sendingQueue = $queue->get('mass_contact_send_message', TRUE);
    $this->mail = $mail_manager;
    $this->entityTypeManager = $entity_type_manager;

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
  public function processMassContactMessage(array $categories, $subject, $body, $format, array $configuration = []) {
    $configuration += $this->getDefaultConfiguration();
    $data = [
      'categories' => $categories,
      'subject' => $subject,
      'body' => $body,
      'format' => $format,
      'configuration' => $configuration,
    ];
    $this->processingQueue->createItem($data);
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
    ];
    return $default;
  }

  /**
   * {@inheritdoc}
   */
  public function queueRecipients(array $category_ids, $subject, $body, $format, array $configuration = []) {

    $recipients = [];
    $data = [
      'subject' => $subject,
      'body' => $body,
      'format' => $format,
      'configuration' => $configuration,
    ];
    $all_recipients = $this->getRecipients($category_ids);
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
    foreach ($all_recipients as $account_id) {
      $recipients[] = $account_id;
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
  public function getRecipients(array $category_ids) {
    /** @var \Drupal\mass_contact\Entity\MassContactCategoryInterface[] $categories */
    $categories = $this->entityTypeManager->getStorage('mass_contact_category')->loadMultiple($category_ids);
    $recipients = [];
    foreach ($categories as $category) {
      foreach ($category->getRecipients() as $plugin_id => $config) {
        $grouping = $category->getGroupingCategories($plugin_id);
        $recipients += $grouping->getRecipients($config['categories']);
      }
    }
    return $recipients;
  }

  /**
   * {@inheritdoc}
   */
  public function sendMessage(array $recipients, $subject, $body, $format, array $configuration = []) {
    $params = [
      'subject' => $subject,
      'body' => $body,
      'format' => $format,
      'configuration' => $configuration,
      'headers' => [],
    ];

    // If utilizing BCC, one email is sent.
    if ($configuration['use_bcc']) {
      $recipient_emails = [];
      foreach ($recipients as $account_id) {
        /** @var \Drupal\user\UserInterface $account */
        if ($account = $this->entityTypeManager->getStorage('user')->load($account_id)) {
          if ($account->isActive()) {
            $recipient_emails[] = $account->getEmail();
          }
        }
      }
      $params['headers']['Bcc'] = implode(',', $recipient_emails);
      $this->mail->mail('mass_contact', 'mass_contact', $configuration['sender_mail'], \Drupal::languageManager()->getDefaultLanguage()->getId(), $params);
    }
    else {
      foreach ($recipients as $account_id) {
        /** @var \Drupal\user\UserInterface $account */
        if ($account = $this->entityTypeManager->getStorage('user')->load($account_id)
        ) {
          // Re-check account is still active.
          if ($account->isActive()) {
            $this->mail->mail('mass_contact', 'mass_contact', $account->getEmail(), $account->language()->getId(), $params);
          }
        }
      }
    }
  }

}
