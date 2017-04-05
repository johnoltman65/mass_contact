<?php

namespace Drupal\mass_contact\Form;

use Drupal\Core\Url;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Email body settings form.
 */
class EmailBodyForm extends SettingsFormBase {

  /**
   * The module handler service.
   *
   * @var \Drupal\Core\Extension\ModuleHandlerInterface
   */
  protected $moduleHandler;

  /**
   * Constructs the email body form.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Extension\ModuleHandlerInterface $module_handler
   *   The module handler service.
   */
  public function __construct(ConfigFactoryInterface $config_factory, ModuleHandlerInterface $module_handler) {
    parent::__construct($config_factory);
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('config.factory'),
      $container->get('module_handler')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'mass_contact_email_body_settings';
  }

  /**
   * {@inheritdoc}
   */
  protected function getConfigKeys() {
    return [
      // @todo
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $form = parent::buildForm($form, $form_state);

    // @todo Port this functionality.
    // @see https://www.drupal.org/node/2867166
    return $form;
    $config = $this->config('mass_contact.settings');

    $mimemail = \Drupal::moduleHandler()->moduleExists('mimemail');
    $token = \Drupal::moduleHandler()->moduleExists('token');

    // Supplemental texts that are prepended and/or appended to every message.
    $form['mass_contact_supplemental_texts'] = [
      '#type' => 'details',
      '#open' => TRUE,
      '#title' => $this->t('Supplemental message body texts'),
      '#description' => $this->t('You may specify additional text to insert before and/or after the message text of every mass email that is sent.'),
    ];

    // @FIXME
    // Could not extract the default value because it is either indeterminate, or
    // not scalar. You'll need to provide a default value in
    // config/install/mass_contact.settings.yml and config/schema/mass_contact.schema.yml.
    $mass_contact_message_prefix = \Drupal::config('mass_contact.settings')->get('mass_contact_message_prefix');
    // @FIXME
    // Could not extract the default value because it is either indeterminate, or
    // not scalar. You'll need to provide a default value in
    // config/install/mass_contact.settings.yml and config/schema/mass_contact.schema.yml.
    $message_suffix = \Drupal::config('mass_contact.settings')->get('message_suffix');

    if ($mimemail) {
      $field_type = 'text_format';

      if (is_array($mass_contact_message_prefix)) {
        $prefix_format = !empty($mass_contact_message_prefix['format']) ? $mass_contact_message_prefix['format'] : NULL;
        $suffix_format = !empty($message_suffix['format']) ? $message_suffix['format'] : NULL;

        if ($token) {
          $prefix_default_value = isset($mass_contact_message_prefix['value']) ? $mass_contact_message_prefix['value'] : t('[current-user:name] has sent you a group email from [site:name].');
          $suffix_default_value = isset($message_suffix['value']) ? $message_suffix['value'] : '';
        }
        else {
          // @FIXME
          // url() expects a route name or an external URI.
          // $prefix_default_value = isset($mass_contact_message_prefix['value']) ? $mass_contact_message_prefix['value'] : t('You were sent a group email from @site.', array('@site' => url(NULL, array('absolute' => TRUE))));
          $suffix_default_value = isset($message_suffix['value']) ? $message_suffix['value'] : '';
        }
      }
      else {
        $prefix_format = !empty($mass_contact_message_prefix) ? $mass_contact_message_prefix : NULL;
        $suffix_format = !empty($message_suffix) ? $message_suffix : NULL;

        if ($token) {
          $prefix_default_value = isset($mass_contact_message_prefix) ? $mass_contact_message_prefix : t('[current-user:name] has sent you a group email from [site:name].');
          $suffix_default_value = isset($message_suffix) ? $message_suffix : '';
        }
        else {
          // @FIXME
          // url() expects a route name or an external URI.
          // $prefix_default_value = isset($mass_contact_message_prefix) ? $mass_contact_message_prefix : t('You were sent a group email from @site.', array('@site' => url(NULL, array('absolute' => TRUE))));
          $suffix_default_value = isset($message_suffix) ? $message_suffix : '';
        }
      }
    }
    else {
      $field_type = 'textarea';
      $prefix_format = NULL;
      $suffix_format = NULL;

      if ($token) {
        $prefix_default_value = isset($mass_contact_message_prefix) ? $mass_contact_message_prefix : t('[current-user:name] has sent you a group email from [site:name].');
        $suffix_default_value = isset($message_suffix) ? $message_suffix : '';
      }
      else {
        // @FIXME
        // url() expects a route name or an external URI.
        // $prefix_default_value = isset($mass_contact_message_prefix) ? $mass_contact_message_prefix : t('You were sent a group email from @site.', array('@site' => url(NULL, array('absolute' => TRUE))));
        $suffix_default_value = isset($message_suffix) ? $message_suffix : '';
      }
    }

    $form['mass_contact_supplemental_texts']['mass_contact_message_prefix'] = [
      '#type' => $field_type,
      '#title' => $this->t('Text to be prepended to all messages'),
      '#default_value' => $prefix_default_value,
      '#format' => $prefix_format,
      '#description' => $this->t('The text you specify in this field will be added to all Mass Contact messages sent out and will be placed before the message text entered in by the sender.'),
    ];

    $form['mass_contact_supplemental_texts']['message_suffix'] = [
      '#type' => $field_type,
      '#title' => t('Text to be appended to all messages'),
      '#default_value' => $suffix_default_value,
      '#format' => $suffix_format,
      '#description' => t('The text you specify in this field will be added to all Mass Contact messages sent out and will be placed after the message text entered in by the sender.'),
    ];

    if ($token) {
      // Display the user documentation of placeholders supported by this module,
      // as a description on the last pattern.
      $form['mass_contact_supplemental_texts']['mass_contact_replacement_tokens'] = [
        '#type' => 'fieldset',
        '#title' => t('Replacement patterns'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#description' => t('You may use any of the following replacements tokens for use in the prefix and/or suffix texts above.'),
      ];
      $form['mass_contact_supplemental_texts']['mass_contact_replacement_tokens']['token_help'] = [
        '#theme' => 'token_tree',
        '#token_types' => ['global'],
      ];
    }

    // HTML options.
    $form['mass_contact_html_settings'] = [
      '#type' => 'fieldset',
      '#title' => t('HTML Settings'),
    ];
    if ($mimemail) {
      // @FIXME
      // Could not extract the default value because it is either indeterminate, or
      // not scalar. You'll need to provide a default value in
      // config/install/mass_contact.settings.yml and config/schema/mass_contact.schema.yml.
      $mass_contact_html_format = \Drupal::config('mass_contact.settings')->get('mass_contact_html_format');
      $form['mass_contact_html_settings']['mass_contact_html_format'] = [
        '#type' => 'text_format',
        '#title' => t('The default text format'),
        '#default_value' => t('This text of this field is not saved or used anywhere.'),
        '#format' => !empty($mass_contact_html_format['format']) ? $mass_contact_html_format['format'] : NULL,
        '#description' => t('This is the text format that will be initially selected. If you do not want to allow HTML messages, then specify a plain text text format and do not aloow it to be overridden below. Keep in mind that the user sending the message may not have access to all the text formats that are available here.'),
      ];
    }
    else {
      $form['mass_contact_html_settings']['mass_contact_no_mimemail'] = [
        '#type' => 'item',
        '#description' => t('This module no longer supports HTML email without the Mime Mail module, which can be found here: http://drupal.org/project/mimemail.'),
      ];
    }

    // Attachment options.
    $form['mass_contact_attachment_settings'] = [
      '#type' => 'fieldset',
      '#title' => t('Attachment Settings'),
    ];
    if ($mimemail) {
      $form['mass_contact_attachment_settings']['number_of_attachments'] = [
        '#type' => 'textfield',
        '#title' => t('Number of attachments'),
        '#default_value' => \Drupal::config('mass_contact.settings')->get('number_of_attachments'),
        '#size' => 10,
        '#description' => t("The number of attachments to allow on the contact form. The maximum number of allowed uploads may be limited by PHP. If necessary, check your system's PHP php.ini file for a max_file_uploads directive to change."),
      ];
      $form['mass_contact_attachment_settings']['attachment_location'] = [
        '#type' => 'textfield',
        '#title' => t('Attachment location'),
        '#default_value' => \Drupal::config('mass_contact.settings')->get('attachment_location'),
        '#description' => t('If a copy of the message is saved as a node, this is the file path where to save the attachment(s) so it can be viewed later. If you specify anything here, it will be a subdirectory of your Public file system path, which is set on !file_conf_page. If you do not specify anything here, all attachments will be saved in the directory specified in the Public file system path.', ['!file_conf_page' => \Drupal::l(t('File system configuration page'), Url::fromRoute('system.file_system_settings'))]),
      ];
    }
    else {
      $form['mass_contact_attachment_settings']['mass_contact_no_mimemail'] = [
        '#type' => 'item',
        '#description' => t('This module no longer supports attachments without the Mime Mail module, which can be found here: http://drupal.org/project/mimemail.'),
      ];
    }

  }

}
