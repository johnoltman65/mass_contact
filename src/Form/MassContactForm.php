<?php

namespace Drupal\mass_contact\Form;

use Drupal\Component\Utility\Xss;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Main form for sending Mass Contact emails.
 */
class MassContactForm extends FormBase {

  /**
   * The mass contact configuration.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   */
  protected $config;

  /**
   * The entity type manager service.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The module handler service.
   *
   * @var \Drupal\Core\Extension\ModuleHandlerInterface
   */
  protected $moduleHandler;

  /**
   * Constructs the Mass Contact form.
   *
   * @param \Drupal\Core\Extension\ModuleHandlerInterface $module_handler
   *   The module handler service.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager service.
   */
  public function __construct(ModuleHandlerInterface $module_handler, EntityTypeManagerInterface $entity_type_manager) {
    $this->config = $this->configFactory()->get('mass_contact.settings');
    $this->entityTypeManager = $entity_type_manager;
    $this->moduleHandler = $module_handler;

  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('module_handler'),
      $container->get('entity_type.manager')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'mass_contact';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $form = [];

    $categories = [];
    $default_category = [];
    $default_category_name = '';

    /** @var \Drupal\mass_contact\Entity\MassContactCategoryInterface $category */
    foreach ($this->entityTypeManager->getStorage('mass_contact_category')->loadMultiple() as $category) {
      if ($this->currentUser()->hasPermission('mass contact send to users in the ' . $category->id() . ' category')) {
        $categories[$category->id()] = $category->label();

        if ($category->getSelected()) {
          $default_category[] = $category->id();
          $default_category_name = $category->label();
        }
      }
    }

    if (count($categories) == 1) {
      $default_category = array_keys($categories);
      $default_category_name = $categories[$default_category[0]];
    }

    if (count($categories) > 0) {
      $form['contact_information'] = [
        '#markup' => Xss::filterAdmin($this->config->get('form_information')),
      ];

      // Add the field for specifying the sender's name.
      $default_sender_name = $this->config->get('default_sender_name');
      if ($default_sender_name) {
        if ($this->currentUser()->hasPermission('mass contact change default sender information')) {
          $form['name'] = [
            '#type' => 'textfield',
            '#title' => $this->t('Your name'),
            '#maxlength' => 255,
            '#default_value' => $default_sender_name,
            '#required' => TRUE,
          ];
        }
        else {
          $form['name'] = [
            '#type' => 'item',
            '#title' => $this->t('Your name'),
            '#value' => $default_sender_name,
          ];
        }
      }
      else {
        $form['name'] = [
          '#type' => 'textfield',
          '#title' => t('Your name'),
          '#maxlength' => 255,
          '#default_value' => $this->currentUser()->getDisplayName(),
          '#required' => TRUE,
        ];
      }

      // Add the field for specifying the sender's email address.
      $default_sender_email = $this->config->get('default_sender_email');
      if ($default_sender_email) {
        if ($this->currentUser()->hasPermission('mass contact change default sender information')) {
          $form['mail'] = [
            '#type' => 'textfield',
            '#title' => $this->t('Your email address'),
            '#maxlength' => 255,
            '#default_value' => $default_sender_email,
            '#required' => TRUE,
          ];
        }
        else {
          $form['mail'] = [
            '#type' => 'item',
            '#title' => $this->t('Your email address'),
            '#value' => $default_sender_email,
          ];
        }
      }
      else {
        $form['mail'] = [
          '#type' => 'textfield',
          '#title' => $this->t('Your email address'),
          '#maxlength' => 255,
          '#default_value' => $this->currentUser()->getEmail(),
          '#required' => TRUE,
        ];
      }

      // Add the field for specifying the category(ies).
      if ((count($categories) > 1) || !isset($default_category)) {
        // Display a choice when one is needed.
        $field_type = $this->config->get('category_display');
        $form['cid'] = [
          '#type' => $field_type,
          '#title' => $this->t('Category'),
          '#default_value' => $default_category,
          '#options' => $categories,
          '#required' => TRUE,
          '#multiple' => TRUE,
        ];
      }
      else {
        // Otherwise, just use the default category.
        $form['cid'] = [
          '#type' => 'value',
          '#value' => $default_category,
        ];
        $form['cid-info'] = [
          '#type' => 'item',
          '#title' => t('Category'),
          '#markup' => $this->t('This message will be sent to all users in the %category category.', ['%category' => $default_category_name]),
        ];
      }

      // Add the field for specifying whether opt-outs are respected or not.
      $optout_setting = $this->config->get('optout_d');

      // Allow users to opt-out of mass emails:
      // 0 => 'No', 1 == 'Yes', 2 == 'Selected categories'.
      if ($optout_setting == 1 || $optout_setting == 2) {
        // @todo https://www.drupal.org/node/2867177
        // Allow to override or respect opt-outs if admin, otherwise use default.
        if ($this->currentUser()->hasPermission('mass contact administer')) {
          $form['optout'] = [
            '#type' => 'checkbox',
            '#title' => t('Respect user opt-outs.'),
            '#default_value' => 1,
          ];
        }
        else {
          $form['optout'] = [
            '#type' => 'hidden',
            '#default_value' => 1,
          ];
        }
      }
      else {
        $form['optout'] = [
          '#type' => 'hidden',
          '#default_value' => 0,
        ];
      }

      // Add the field for specifying whether the recipients are in the To or
      // BCC field of the message.
      // Check if the user is allowed to override the BCC setting.
      if ($this->currentUser()->hasPermission('mass contact override bcc')) {
        $form['bcc'] = [
          '#type' => 'checkbox',
          '#title' => t('Send as BCC (hide recipients)'),
          '#default_value' => $this->config->get('bcc_d'),
        ];
      }
      // If not, then just display the BCC info.
      else {
        $form['bcc'] = [
          '#type' => 'value',
          '#value' => $this->config->get('bcc_d'),
        ];
        $form['bcc-info'] = [
          '#type' => 'item',
          '#title' => t('Send as BCC (hide recipients)'),
          '#markup' => $this->config->get('bcc_d')
          ? $this->t('Recipients will be hidden from each other.')
          : $this->t('Recipients will NOT be hidden from each other.'),
        ];
      }

      // Add the field for specifying the subject of the message.
      $form['subject'] = [
        '#type' => 'textfield',
        '#title' => $this->t('Subject'),
        '#maxlength' => 255,
        '#required' => TRUE,
      ];

      // Add the field for specifying the body and text format of the message.
      // Get the HTML input format setting and the corresponding name.
      // Get the admin specified default text format.
      $default_filter_format = $this->config->get('message_format');

      // Check if the user is allowed to override the text format.
      $form['body']['message'] = [
        '#type' => 'text_format',
        '#title' => t('Message'),
        '#format' => $default_filter_format ?: filter_default_format(),
        '#rows' => 12,
        '#required' => TRUE,
      ];
      if (!$this->currentUser()->hasPermission('mass contact override text format')) {
        // The user is not allowed to override the text format, so lock it down
        // to the default one.
        $form['body']['message']['#allowed_formats'] = [$default_filter_format ?: filter_default_format()];
      }

      if (!$this->moduleHandler->moduleExists('mimemail') && !$this->moduleHandler->moduleExists('swiftmailer')) {
        // No HTML email handling, lock down to plain text.
        $form['body']['message']['#allowed_formats'] = ['plain_text'];
        $form['body']['message']['#format'] = 'plain_text';
      }

      // If the user has access, add the field for specifying the attachment.
      if ($this->moduleHandler->moduleExists('mimemail') || $this->moduleHandler->moduleExists('swiftmailer')) {
        // @todo Port message body prefix/suffix.
        // @see https://www.drupal.org/node/2867166
        if ($this->currentUser()->hasPermission('mass contact include attachments')) {
          for ($i = 1; $i <= \Drupal::config('mass_contact.settings')->get('number_of_attachments'); $i++) {
            $form['attachment_' . $i] = [
              '#type' => 'file',
              '#title' => t('Attachment #!number', ['!number' => $i]),
            ];
          }
        }
      }

      // We do not allow anonymous users to send themselves a copy because it
      // can be abused to spam people.
      // @todo Why are anonymous users allowed to hit this form at all?!
      if ($this->currentUser()->id()) {
        $form['copy'] = [
          '#type' => 'checkbox',
          '#title' => t('Send yourself a copy.'),
        ];
      }

      // Add the field for specifying whether to save the message as a node or
      // not.
      if ($this->currentUser()->hasPermission('mass contact archive messages')) {
        // Check if the user is allowed to override the node copy setting.
        if (\Drupal::currentUser()->hasPermission('mass contact override archiving')) {
          $form['nodecc'] = [
            '#type' => 'checkbox',
            '#title' => $this->t('Archive a copy of this message on this website'),
            '#default_value' => $this->config->get('nodecc_d'),
          ];
        }
        // If not, then do it or not based on the administrative setting.
        else {
          $form['nodecc'] = [
            '#type' => 'hidden',
            '#default_value' => $this->config->get('nodecc_d'),
          ];
          $form['nodecc_notice'] = [
            '#type' => 'item',
            '#title' => $this->t('Archive a copy of this message on this website'),
            '#markup' => $this->t('A copy of this message will !not be archived on this website.', ['!not' => $this->config->get('nodecc_d') ? '' : 'not']),
          ];
        }
      }
      // If not, then do it or not based on the administrative setting.
      else {
        $form['nodecc'] = [
          '#type' => 'hidden',
          '#default_value' => $this->config->get('nodecc_d'),
        ];
        $form['nodecc_notice'] = [
          '#type' => 'item',
          '#title' => $this->t('Archive a copy of this message on this website'),
          '#markup' => $this->t('A copy of this message will !not be archived on this website.', ['!not' => $this->config->get('nodecc_d') ? '' : 'not']),
        ];
      }

      // Add the submit button.
      $form['submit'] = [
        '#type' => 'submit',
        '#value' => $this->t('Send email'),
      ];
    }
    else {
      drupal_set_message($this->t('No categories found!'), 'error');
      $form['error'] = [
        '#markup' => $this->t('Either <a href="@url">create at least one category</a> of users to send to, or contact your system administer for access to the existing categories.', ['@url' => Url::fromRoute('entity.mass_contact_category.collection')->toString()]),
      ];
    }

    if ($this->currentUser()->hasPermission('mass contact administer')) {
      $tasks = [];
      if ($this->currentUser()->hasPermission('administer permissions')) {
        $tasks[] = Link::createFromRoute($this->t('Set Mass Contact permissions'), 'user.admin_permissions', [], ['fragment' => 'module-mass_contact'])->toRenderable();
      }
      $tasks[] = Link::createFromRoute($this->t('List current categories'), 'entity.mass_contact_category.collection')->toRenderable();
      $tasks[] = Link::createFromRoute($this->t('Add new category'), 'entity.mass_contact_category.add_form')->toRenderable();
      $tasks[] = Link::createFromRoute($this->t('Configure Mass Contact settings'), 'mass_contact.settings')->toRenderable();
      $tasks[] = Link::createFromRoute($this->t('Help'), 'help.page', ['name' => 'mass_contact'])->toRenderable();

      $form['tasklist'] = [
        '#type' => 'details',
        // Open if there are no categories.
        '#open' => empty($categories),
        '#title' => $this->t('Related tasks'),
      ];
      $form['tasklist']['tasks'] = [
        '#theme' => 'item_list',
        '#items' => $tasks,
      ];
    }

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

  }

}
