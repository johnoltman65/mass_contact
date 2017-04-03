<?php

namespace Drupal\mass_contact\Form;

use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;

/**
 * Admin settings form for Mass Contact.
 */
class AdminSettingsForm extends ConfigFormBase {

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return ['mass_config.settings'];
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'mass_config_admin_settings';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $form = parent::buildForm($form, $form_state);

    $config = $this->config('mass_contact.settings');
    $form['form_information'] = [
     '#type' => 'textarea',
     '#title' => t('Additional information for Mass Contact form'),
     '#default_value' => $config->get('form_information'),
     '#description' => $this->t('Information to show on the <a href=":url">Mass Contact page</a>.', [':url' => Url::fromRoute('mass_contact')->toString()]),
    ];

    // Rate limiting options.
    $form['limiting_options'] = [
      '#type' => 'details',
      '#title' => $this->t('Rate limiting options'),
      '#description' => $this->t('By combining the two options below, messages sent through this module will be queued to be sent drung cron runs. Keep in mind that if you set your number of recipients to be the same as your limit, messages from this or other modules may be blocked by your hosting provider.'),
    ];
    // The maximum number of users to send to at one time.
    $form['limiting_options']['recipient_limit'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Maximum number of recipients before splitting up the email'),
      '#size' => 10,
      '#default_value' => $config->get('recipient_limit'),
      '#description' => $this->t('This is a workaround for server-side limits on the number of recipients in a single mail message. Once this limit is reached, the recipient list will be broken up and multiple copies of the message will be sent out until all recipients receive the mail. Setting this to 0 (zero) will turn this feature off.'),
      '#required' => TRUE,
    ];
    // The maximum number of users to send to at one time.
    $form['mass_contact_rate_limiting_options']['send_with_cron'] = [
      '#type' => 'checkbox',
      '#title' => $this->t('Send messages with Cron'),
      '#default_value' => $config->get('send_with_cron'),
      '#description' => $this->t('This is another workaround for server-side limits. Check this box to delay sending until the next Cron run(s).'),
    ];

    // Opt out options.
    $form['mass_contact_optout_options'] = [
      '#type' => 'details',
      '#title' => $this->t('Opt-out options'),
    ];
    $form['mass_contact_optout_options']['optout_d'] = [
      '#type' => 'radios',
      '#title' => $this->t('Allow users to opt-out of mass email messages'),
      '#default_value' => $config->get('optout_d'),
      '#options' => [
        0 => 'No',
        1 => 'Yes',
        2 => 'Selected categories',
      ],
      '#description' => $this->t("Allow users to opt-out of receiving mass email messages. If 'No' is chosen, then the site's users will not be able to opt-out of receiving mass email messages. If 'Yes' is chosen, then the site's users will be able to opt-out of receiving mass email messages, and they will not receive any from any category. If 'Selected categories' is chosen, then the site's users will be able to opt-out of receiving mass email messages from which ever categories they choose."),
    ];

    // @FIXME
// Could not extract the default value because it is either indeterminate, or
// not scalar. You'll need to provide a default value in
// config/install/mass_contact.settings.yml and config/schema/mass_contact.schema.yml.
    $form['mass_contact_optout_options']['mass_contact_optout_message'] = [
      '#type' => 'textarea',
      '#title' => $this->t('The message to display to users when giving them the option to opt out'),
      '#default_value' => $config->get('mass_contact_optout_message'),
      '#description' => $this->t('This is the message users will see in thier account settings page when they are presented with a list of categories to opt out of.'),
    ];

    // Node copy options.
    $form['nodecc_d'] = [
      '#type' => 'checkbox',
      '#title' => $this->t('Archive messages by saving a copy as a node'),
      '#default_value' => \Drupal::config('mass_contact.settings')->get('nodecc_d'),
    ];

    // Flood control options.
    $form['hourly_threshold'] = [
      '#type' => 'textfield',
      '#size' => 10,
      '#title' => $this->t('Hourly threshold'),
      '#default_value' => $config->get('hourly_threshold'),
      '#description' => $this->t('The maximum number of Mass Contact form submissions a user can perform per hour.'),
    ];

    return $form;
  }

}
