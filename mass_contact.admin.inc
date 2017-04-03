<?php
/**
 * @file
 * The administrative settings pages.
 */

/**
 * Displays a list of all existing categories.
 *
 * @return array
 *   The themed page listing all current categories.
 */
function mass_contact_admin_categories() {
  $rows = array();

  $results = db_select('mass_contact', 'mc')
    ->fields('mc', array('cid', 'category', 'recipients', 'selected'))
    ->orderBy('category', 'ASC')
    ->execute();
  foreach ($results as $category) {
    // Prepare the string for the Recipients column.
    // @FIXME
// Most CTools APIs have been moved into core.
// 
// @see https://www.drupal.org/node/2164623
// ctools_include('plugins');

    // Get the information about all plugins that implemnent this type of
    // plugin.
    $plugins = \Drupal::service('plugin.manager.mass_contact.grouping_method')->getDefinitions();
    $recipients = array();
    foreach ($plugins as $plugin) {
      // Get the admin categories function name for this particular
      // implementation.
      $function = ctools_plugin_get_function($plugin, 'mass_contact_categories');
      // Call the plugin function to get the list of categories.
      $recipients[] = $function(unserialize($category->recipients));
    }

    // @FIXME
// l() expects a Url object, created from a route name or external URI.
// $rows[] = array(
//       check_plain($category->category),
//       implode('<br/>', array_filter($recipients)),
//       ($category->selected ? t('Yes') : t('No')),
//       l(t('edit'), 'admin/config/system/mass_contact/edit/' . $category->cid),
//       l(t('delete'), 'admin/config/system/mass_contact/delete/' . $category->cid),
//     );

  }

  $header = array(
    t('Category'),
    t('Recipients'),
    t('Selected'),
    array(
      'data' => t('Operations'),
      'colspan' => 2,
    ),
  );

  // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// return theme('table', array('header' => $header, 'rows' => $rows));

}


/**
 * Displays a form to add or edit a category.
 *
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 * @param integer $cid
 *   The id of the category to edit. If NULL, then add rather than edit.
 *
 * @return array
 *   An associative array that defines the form to be built.
 */
function mass_contact_admin_edit($form, $form_state, $cid = NULL) {
  // Initialize the array.
  $edit = array(
    'category' => '',
    'recipients' => '',
    'selected' => '',
    'cid' => '',
  );

  if (arg(4) == "edit" && $cid > 0) {
    // Get the information about the category being edited.
    $edit = db_select('mass_contact', 'mc')
      ->fields('mc')
      ->condition('cid', $cid)
      ->execute()
      ->fetchAssoc();
  }

  $form['category'] = array(
    '#type' => 'textfield',
    '#title' => t('Category'),
    '#maxlength' => 255,
    '#default_value' => $edit['category'],
    '#description' => t("Will appear in the subject of your email as [category]."),
    '#required' => TRUE,
  );

  $form['recipients'] = array('#tree' => TRUE);

  // Add form elements provided by grouping_method plugins.
  // @FIXME
// Most CTools APIs have been moved into core.
// 
// @see https://www.drupal.org/node/2164623
// ctools_include('plugins');

  // Get the information about all plugins that implemnent this type of plugin.
  $plugins = \Drupal::service('plugin.manager.mass_contact.grouping_method')->getDefinitions();
  foreach ($plugins as $plugin_name => $plugin) {
    // Get the admin edit function name for this particular implementation.
    $function = ctools_plugin_get_function($plugin, 'mass_contact_admin_edit');
    // Call the plugin function to add the form snippet(s).
    $form['recipients'][$plugin_name] = $function(unserialize($edit['recipients']));
  }

  $form['selected_categories'] = array(
    '#type' => 'fieldset',
    '#title' => t('Selected categories'),
  );
  $form['selected_categories']['selected'] = array(
    '#type' => 'select',
    '#title' => t('Selected'),
    '#options' => array(
      '0' => t('No'),
      '1' => t('Yes'),
    ),
    '#default_value' => $edit['selected'],
    '#description' => t('Set this to <em>Yes</em> if you would like this category to be selected by default.'),
  );
  $form['selected_categories']['reset_selected'] = array(
    '#type' => 'checkbox',
    '#title' => t('Reset all previously selected categories to <em>No</em>'),
  );

  $form['cid'] = array(
    '#type' => 'value',
    '#value' => $edit['cid'],
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Validates the submission of the category add/edit page.
 *
 * @param array $form
 *   An associative array containing the structure of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_admin_edit_validate($form, &$form_state) {
  // Execute validation callbacks for each plugin.
  // @FIXME
// Most CTools APIs have been moved into core.
// 
// @see https://www.drupal.org/node/2164623
// ctools_include('plugins');

  // Get the information about all plugins that implemnent this type of plugin.
  $plugins = \Drupal::service('plugin.manager.mass_contact.grouping_method')->getDefinitions();
  $values_empty = TRUE;
  foreach ($plugins as $plugin) {
    // Get the admin edit validate function name for this particular
    // implementation.
    $function = ctools_plugin_get_function($plugin, 'mass_contact_admin_edit_validate');
    if ($values_empty) {
      // Call the plugin function to validate the form.
      $values_empty = $function($form, $form_state);
    }
  }
  // If all validation callbacks return TRUE that means that user
  // hasn't selected any selection rules.
  if ($values_empty) {
    form_set_error('recipients', t('You must check one or more recipients.'));
  }
}

/**
 * Processes the adding or editing of a category.
 *
 * @param array $form
 *   An associative array containing the structure of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_admin_edit_submit($form, &$form_state) {
  if ($form_state['values']['reset_selected']) {
    // Unselect all other contact categories.
    db_update('mass_contact')
      ->fields(array('selected' => 0))
      ->execute();
  }

  // Collect the recipients.
  // @FIXME
// Most CTools APIs have been moved into core.
// 
// @see https://www.drupal.org/node/2164623
// ctools_include('plugins');

  // Get the information about all plugins that implemnent this type of plugin.
  $plugins = \Drupal::service('plugin.manager.mass_contact.grouping_method')->getDefinitions();
  $recipients = array();
  foreach ($plugins as $plugin_name => $plugin) {
    // Get the recipient list function name for this particular implementation.
    $function = ctools_plugin_get_function($plugin, 'mass_contact_admin_edit_submit');
    // Call the plugin function to submit the form data.
    $recipients[$plugin_name] = $function($form, $form_state);
  }

  $recipients = serialize($recipients);

  if (!isset($form_state['values']['reply'])) {
    $form_state['values']['reply'] = '';
  }
  if (!isset($form_state['values']['weight'])) {
    $form_state['values']['weight'] = 0;
  }

  $record = array(
    'category' => $form_state['values']['category'],
    'recipients' => $recipients,
    'reply' => $form_state['values']['reply'],
    'weight' => $form_state['values']['weight'],
    'selected' => $form_state['values']['selected'],
  );
  if (arg(4) == 'add') {
    \Drupal::database()->insert('mass_contact')->fields($record)->execute();
    drupal_set_message(t('Category %category has been added.', array('%category' => $form_state['values']['category'])));
    // @FIXME
// l() expects a Url object, created from a route name or external URI.
// watchdog('mass_contact', 'Mass Contact form: category %category added.', array('%category' => $form_state['values']['category']), WATCHDOG_NOTICE, l(t('view'), 'admin/config/system/mass_contact'));

  }
  else {
    $record['cid'] = $form_state['values']['cid'];
    \Drupal::database()->merge('mass_contact')->fields($record)->key(array('cid'))->execute();
    drupal_set_message(t('Category %category has been updated.', array('%category' => $form_state['values']['category'])));
    // @FIXME
// l() expects a Url object, created from a route name or external URI.
// watchdog('mass_contact', 'Mass Contact form: category %category updated.', array('%category' => $form_state['values']['category']), WATCHDOG_NOTICE, l(t('view'), 'admin/config/system/mass_contact'));

  }

  if (\Drupal::moduleHandler()->moduleExists('adminrole')) {
    adminrole_update_permissions();
  }

  $form_state['redirect'] = 'admin/config/system/mass_contact';
}


/**
 * Displays a form to select a category to delete.
 *
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 * @param integer $cid
 *   The id of the category to delete.
 *
 * @return array
 *   A confirmation form for the user to acknowledge.
 */
function mass_contact_admin_delete($form, $form_state, $cid = NULL) {
  $info = db_select('mass_contact', 'mc')
    ->fields('mc', array('category'))
    ->condition('cid', $cid)
    ->execute()
    ->fetchObject();
  if ($info) {
    $form['category'] = array(
      '#type' => 'value',
      '#value' => $info->category,
    );

    return confirm_form($form, t('Are you sure you want to delete %category?', array('%category' => $info->category)), 'admin/config/system/mass_contact', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
  }
  else {
    drupal_set_message(t('Category not found.'), 'error');
    drupal_goto('admin/config/system/mass_contact');
  }
}

/**
 * Does the actual deleting of the category.
 *
 * @param array $form
 *   An associative array containing the structure of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_admin_delete_submit($form, &$form_state) {
  db_delete('mass_contact')
    ->condition('cid', arg(5))
    ->execute();
  drupal_set_message(t('Category %category has been deleted.', array('%category' => $form_state['values']['category'])));
  \Drupal::logger('mass_contact')->notice('Mass Contact form: category %category deleted.', array('%category' => $form_state['values']['category']));
  $form_state['redirect'] = 'admin/config/system/mass_contact';
}


/**
 * Miscellaneous administration settings form.
 *
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @return array
 *   An associative array that defines the form to be built.
 */
function mass_contact_admin_settings_misc($form, $form_state) {

  return system_settings_form($form);
}


/**
 * Message header administration settings form.
 *
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @return array
 *   An associative array that defines the form to be built.
 */
function mass_contact_admin_settings_header($form, $form_state) {
  // The default character set.
  $form['character_set'] = array(
    '#type' => 'textfield',
    '#title' => t('Character set'),
    '#default_value' => \Drupal::config('mass_contact.settings')->get('character_set'),
    '#description' => t('You may specify an alternate character set to use when sending emails. If left blank, the default of UTF-8 will be used. If you are unsure of what to put here, then leave it blank. Caution: setting this may not get you the results you desire. Other modules may come along and change that value after it has been set by this module.'),
  );

  // The sender's name and email address.
  $form['mass_contact_default_sender'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default sender information'),
    '#description' => t('If anything is specified in here, it is used in place of the "Your name" and "Your email address" fields when sending the mass email. Otherwise, the sender\'s name and email address will be the default values. You must fill in both values, if you want to specify a default.'),
  );
  $form['mass_contact_default_sender']['default_sender_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Default sender name'),
    '#default_value' => \Drupal::config('mass_contact.settings')->get('default_sender_name'),
    '#size' => 60,
    '#maxlength' => 128,
    '#description' => t('The optional user name to send email as. Replaces the "Your name" value when sending mass emails.'),
  );
  $form['mass_contact_default_sender']['default_sender_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Default sender email address'),
    '#default_value' => \Drupal::config('mass_contact.settings')->get('default_sender_email'),
    '#size' => 60,
    '#maxlength' => 128,
    '#description' => t('The optional user email address to send email as. Replaces the "Your email address" value when sending mass emails.'),
  );

  // Category options.
  $form['category_display'] = array(
    '#type' => 'radios',
    '#title' => t('Field to use to display the categories'),
    '#default_value' => \Drupal::config('mass_contact.settings')->get('category_display'),
    '#options' => array(
      'select' => 'Select list',
      'checkboxes' => 'Check boxes',
    ),
    '#description' => t("Select the form field to use to display the available categories to the message sender."),
  );

  // Sender name options.
  $form['mass_contact_include_name'] = array(
    '#type' => 'fieldset',
    '#title' => t('Include names with email addresses'),
    '#description' => t("Checking either of the boxes below will include the name along with the email address, in the form of '%address'. If you have problems with sending mail, especially when your site is on a Windows server, try unchecking both checkboxes.", array('%address' => 'User name <email.address@example.com>')),
  );
  $form['mass_contact_include_name']['include_from_name'] = array(
    '#type' => 'checkbox',
    '#title' => t('Include the name for the sender'),
    '#default_value' => \Drupal::config('mass_contact.settings')->get('include_from_name'),
  );
  $form['mass_contact_include_name']['include_to_name'] = array(
    '#type' => 'checkbox',
    '#title' => t('Include the names for the recipients'),
    '#default_value' => \Drupal::config('mass_contact.settings')->get('include_to_name'),
    '#description' => t("The name used for the recipients will be their site login ID."),
  );

  // BCC options.
  $form['bcc_d'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send as BCC (hide recipients) by default'),
    '#default_value' => \Drupal::config('mass_contact.settings')->get('bcc_d'),
  );

  // More category options.
  $form['category_override'] = array(
    '#type' => 'checkbox',
    '#title' => t('Include category in subject line'),
    '#default_value' => \Drupal::config('mass_contact.settings')->get('category_override'),
    '#description' => t("If you choose this option, the category name will be printed in square brackets preceeding the message sender's subject.
                        If the message sender has multiple categories selected with this option choosen, each category will be processed separately.
                        If you do not choose this option and the message sender has multiple categories selected, all users will be grouped together and the message will be sent to everyone as one group, thus reducing the likelihood of sending duplicates."),
  );

  return system_settings_form($form);
}

/**
 * Validates the message header administration settings form.
 *
 * @param array $form
 *   An associative array containing the structure of the form.
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_admin_settings_header_validate($form, &$form_state) {
  if (!empty($form_state['values']['default_sender_name'])) {
    if (empty($form_state['values']['default_sender_email'])) {
      form_set_error('default_sender_email', t('If you are going to specify default user settings, you must specify both a user name and a user email address.'));
    }
  }

  if (!empty($form_state['values']['default_sender_email'])) {
    if (empty($form_state['values']['default_sender_name'])) {
      form_set_error('default_sender_name', t('If you are going to specify default user settings, you must specify both a user name and a user email address.'));
    }
  }
}


/**
 * Message body administration settings form.
 *
 * @param array $form_state
 *   A keyed array containing the current state of the form.
 *
 * @return array
 *   An associative array that defines the form to be built.
 */
function mass_contact_admin_settings_body($form, $form_state) {
  $mimemail = \Drupal::moduleHandler()->moduleExists('mimemail');
  $token = \Drupal::moduleHandler()->moduleExists('token');

  // Supplemental texts that are prepended and/or appended to every message.
  $form['mass_contact_supplemental_texts'] = array(
    '#type' => 'fieldset',
    '#title' => t('Supplemental message body texts'),
    '#description' => t('You may specify additional text to insert before and/or after the message text of every mass email that is sent.'),
  );

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

  $form['mass_contact_supplemental_texts']['mass_contact_message_prefix'] = array(
    '#type' => $field_type,
    '#title' => t('Text to be prepended to all messages'),
    '#default_value' => $prefix_default_value,
    '#format' => $prefix_format,
    '#description' => t('The text you specify in this field will be added to all Mass Contact messages sent out and will be placed before the message text entered in by the sender.'),
  );

  $form['mass_contact_supplemental_texts']['message_suffix'] = array(
    '#type' => $field_type,
    '#title' => t('Text to be appended to all messages'),
    '#default_value' => $suffix_default_value,
    '#format' => $suffix_format,
    '#description' => t('The text you specify in this field will be added to all Mass Contact messages sent out and will be placed after the message text entered in by the sender.'),
  );

  if ($token) {
    // Display the user documentation of placeholders supported by this module,
    // as a description on the last pattern.
    $form['mass_contact_supplemental_texts']['mass_contact_replacement_tokens'] = array(
      '#type' => 'fieldset',
      '#title' => t('Replacement patterns'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#description' => t('You may use any of the following replacements tokens for use in the prefix and/or suffix texts above.'),
    );
    $form['mass_contact_supplemental_texts']['mass_contact_replacement_tokens']['token_help'] = array(
      '#theme' => 'token_tree',
      '#token_types' => array('global'),
    );
  }

  // HTML options.
  $form['mass_contact_html_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('HTML Settings'),
  );
  if ($mimemail) {
    // @FIXME
// Could not extract the default value because it is either indeterminate, or
// not scalar. You'll need to provide a default value in
// config/install/mass_contact.settings.yml and config/schema/mass_contact.schema.yml.
$mass_contact_html_format = \Drupal::config('mass_contact.settings')->get('mass_contact_html_format');
    $form['mass_contact_html_settings']['mass_contact_html_format'] = array(
      '#type' => 'text_format',
      '#title' => t('The default text format'),
      '#default_value' => t('This text of this field is not saved or used anywhere.'),
      '#format' => !empty($mass_contact_html_format['format']) ? $mass_contact_html_format['format'] : NULL,
      '#description' => t('This is the text format that will be initially selected. If you do not want to allow HTML messages, then specify a plain text text format and do not aloow it to be overridden below. Keep in mind that the user sending the message may not have access to all the text formats that are available here.'),
    );
  }
  else {
    $form['mass_contact_html_settings']['mass_contact_no_mimemail'] = array(
      '#type' => 'item',
      '#description' => t('This module no longer supports HTML email without the Mime Mail module, which can be found here: http://drupal.org/project/mimemail.'),
    );
  }

  // Attachment options.
  $form['mass_contact_attachment_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Attachment Settings'),
  );
  if ($mimemail) {
    $form['mass_contact_attachment_settings']['number_of_attachments'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of attachments'),
      '#default_value' => \Drupal::config('mass_contact.settings')->get('number_of_attachments'),
      '#size' => 10,
      '#description' => t("The number of attachments to allow on the contact form. The maximum number of allowed uploads may be limited by PHP. If necessary, check your system's PHP php.ini file for a max_file_uploads directive to change."),
    );
    $form['mass_contact_attachment_settings']['attachment_location'] = array(
      '#type' => 'textfield',
      '#title' => t('Attachment location'),
      '#default_value' => \Drupal::config('mass_contact.settings')->get('attachment_location'),
      '#description' => t('If a copy of the message is saved as a node, this is the file path where to save the attachment(s) so it can be viewed later. If you specify anything here, it will be a subdirectory of your Public file system path, which is set on !file_conf_page. If you do not specify anything here, all attachments will be saved in the directory specified in the Public file system path.', array('!file_conf_page' => \Drupal::l(t('File system configuration page'), \Drupal\Core\Url::fromRoute('system.file_system_settings')))),
    );
  }
  else {
    $form['mass_contact_attachment_settings']['mass_contact_no_mimemail'] = array(
      '#type' => 'item',
      '#description' => t('This module no longer supports attachments without the Mime Mail module, which can be found here: http://drupal.org/project/mimemail.'),
    );
  }

  return system_settings_form($form);
}
