<?php

/**
 * @file
 * This is the main code file for the Mass Contact module.
 *
 * This module enables users to send messages (contact) to multiple users
 * through selected roles.
 */

/* **************************************************************
 *
 * Functions for listing, adding/editing and deleting categories.
 *
 * *************************************************************/

/**
 * Displays a list of all existing categories.
 *
 * @return
 *   The themed page listing all current categories.
 */
function mass_contact_admin_categories() {
  $rows = array();

  $results = db_select('mass_contact', 'mc')
    ->fields('mc', array('cid', 'category', 'recipients', 'selected'))
    ->orderBy('category', 'ASC')
    ->execute();
  foreach ($results as $category) {
    $roles = array();

    foreach (explode(',', $category->recipients) as $rid) {
      $role = db_select('role', 'r')
        ->fields('r', array('name'))
        ->condition('rid', $rid)
        ->execute()
        ->fetchObject();
      $roles[] = ($role->name);
    }

    $rolenames = implode(', ', $roles);
    $rows[] = array(
      $category->category,
      $rolenames,
      ($category->selected ? t('Yes') : t('No')),
      l(t('edit'), 'admin/config/system/mass_contact/edit/' . $category->cid),
      l(t('delete'), 'admin/config/system/mass_contact/delete/' . $category->cid)
    );
  }

  $header = array(
    t('Category'),
    t('Recipients'),
    t('Selected'),
    array(
      'data' => t('Operations'),
      'colspan' => 2,
    )
  );

  return theme('table', array('header' => $header, 'rows' => $rows));
} // End of mass_contact_admin_categories().


/**
 * Displays a form to add or edit a category.
 *
 * @param form_state
 *   A keyed array containing the current state of the form.
 * @param cid
 *   The id of the category to edit. If NULL, then add rather than edit.
 *
 * @return
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
    '#description' => t("Will appear in the subject of your e-mail as [category]."),
    '#required' => TRUE,
  );

  // Get a list of all roles, except for the anonymous user role.
  $allroles = db_select('role', 'r')
    ->fields('r', array('rid', 'name'))
    ->condition('rid', 1, '>')
    ->orderBy('name', 'ASC')
    ->execute();
  foreach ($allroles as $roleobj) {
    $onerid = $roleobj->rid;
    $onename = $roleobj->name;
    $rolesarray[$onerid] = $onename;
  }
  $form['recipients'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Roles to receive e-mail'),
    '#options' => $rolesarray,
    '#default_value' => explode(',', $edit['recipients']),
    '#description' => t('These roles will be added to the mailing list. Note: if you check "authenticated users", other roles will not be added, as they will receive the e-mail anyway.'),
  );

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
    '#title' => t('Reset all previously selected categories to <em>No</em>.'),
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
} // End of mass_contact_admin_edit().

/**
 * Validates the submission of the category add/edit page.
 *
 * @param form
 *   An associative array containing the structure of the form.
 * @param form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_admin_edit_validate($form, &$form_state) {
  $recipients = $form_state['values']['recipients'];
  foreach ($recipients as $checkr) {
    if ($checkr > 1) {
      return;
    }
  }
  form_set_error('recipients', t('You must check one or more recipients.'));
} // End of mass_contact_admin_edit_validate().

/**
 * Processes the adding or editing of a category.
 *
 * @param form
 *   An associative array containing the structure of the form.
 * @param form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_admin_edit_submit($form, &$form_state) {
  if ($form_state['values']['reset_selected']) {
    // Unselect all other contact categories.
    db_update('mass_contact')
      ->fields(array('selected' => 0))
      ->execute();
  }

  // Remove 0s for unselected roles, convert to csv.
  $recipients = $form_state['values']['recipients'];

  // If all authenticated users are already added, remove all roles.
  if ($recipients[2] == 2) {
    foreach ($recipients as $checkr) {
      if ($checkr > 2) {
        $recipients[$checkr] = 0;
      }
    }
  }

  // Remove roles that were not selected.
  foreach ($recipients as $recip) {
    if ($recip != 0) {
      $newformrec[] = $recip;
    }
  }
  $form_state['values']['recipients'] = implode(',', $newformrec);

  if (!isset($form_state['values']['reply'])) {
    $form_state['values']['reply'] = '';
  }
  if (!isset($form_state['values']['weight'])) {
    $form_state['values']['weight'] = 0;
  }

  if (arg(4) == 'add') {
    db_insert('mass_contact')
      ->fields(array(
        'category' => $form_state['values']['category'],
        'recipients' => $form_state['values']['recipients'],
        'reply' => $form_state['values']['reply'],
        'weight' => $form_state['values']['weight'],
        'selected' => $form_state['values']['selected'],
      ))
      ->execute();
    drupal_set_message(t('Category %category has been added.', array('%category' => $form_state['values']['category'])));
    watchdog('mass_contact', 'Mass Contact form: category %category added.', array('%category' => $form_state['values']['category']), WATCHDOG_NOTICE, l(t('view'), 'admin/config/system/mass_contact'));
  }
  else {
    db_update('mass_contact')
      ->fields(array(
        'category' => $form_state['values']['category'],
        'recipients' => $form_state['values']['recipients'],
        'reply' => $form_state['values']['reply'],
        'weight' => $form_state['values']['weight'],
        'selected' => $form_state['values']['selected'],
      ))
      ->condition('cid', $form_state['values']['cid'])
      ->execute();
    drupal_set_message(t('Category %category has been updated.', array('%category' => $form_state['values']['category'])));
    watchdog('mass_contact', 'Mass Contact form: category %category updated.', array('%category' => $form_state['values']['category']), WATCHDOG_NOTICE, l(t('view'), 'admin/config/system/mass_contact'));
  }

  if (module_exists('adminrole')) {
    adminrole_update_permissions();
  }

  $form_state['redirect'] = 'admin/config/system/mass_contact';
} // End of mass_contact_admin_edit_submit().


/**
 * Displays a form to select a category to delete.
 *
 * @param form_state
 *   A keyed array containing the current state of the form.
 * @param cid
 *   The id of the category to delete.
 *
 * @return
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
} // End of mass_contact_admin_delete().

/**
 * Does the actual deleting of the category.
 *
 * @param form
 *   An associative array containing the structure of the form.
 * @param form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_admin_delete_submit($form, &$form_state) {
  db_delete('mass_contact')
    ->condition('cid', arg(5))
    ->execute();
  drupal_set_message(t('Category %category has been deleted.', array('%category' => $form_state['values']['category'])));
  watchdog('mass_contact', 'Mass Contact form: category %category deleted.', array('%category' => $form_state['values']['category']));
  $form_state['redirect'] = 'admin/config/system/mass_contact';
} // End of mass_contact_admin_delete_submit().


/* ***********************************************
 *
 * Functions for handling administrative settings.
 *
 * **********************************************/


/**
 * Miscellaneous administration settings form.
 *
 * @param form_state
 *   A keyed array containing the current state of the form.
 *
 * @return
 *   An associative array that defines the form to be built.
 */
function mass_contact_admin_settings_misc($form, $form_state) {
  ////////////////////////////////////////////////////////////
  // Instructional text.
  $form['mass_contact_form_information'] = array(
    '#type' => 'textarea',
    '#title' => t('Additional information for Mass Contact form'),
    '#default_value' => variable_get('mass_contact_form_information', t('Send e-mails using the following form.')),
    '#description' => t('Information to show on the <a href="@form">Mass Contact page</a>.', array('@form' => url('mass_contact'))),
  );

  ////////////////////////////////////////////////////////////
  // The maximum number of users to send to at one time.
  $form['mass_contact_recipient_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of recipients before splitting up the e-mail'),
    '#size' => 10,
    '#default_value' => variable_get('mass_contact_recipient_limit', 0),
    '#description' => t('This is a workaround for server-side limits on the number of recipients in a single mail message. Once this limit is reached, the recipient list will be broken up and multiple copies of the message will be sent out until all recipients receive the mail. Setting this to "0" will turn off this feature.'),
    '#required' => TRUE,
  );

  ////////////////////////////////////////////////////////////
  // Opt out options.
  $form['mass_contact_optout_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Opt-out options'),
  );
  $form['mass_contact_optout_options']['mass_contact_optout_d'] = array(
    '#type' => 'radios',
    '#title' => t('Allow users to opt-out of mass e-mails'),
    '#default_value' => variable_get('mass_contact_optout_d', 0),
    '#options' => array(
      0 => 'No',
      1 => 'Yes',
      2 => 'Selected categories',
    ),
    '#description' => t("Allow users to opt-out of receiving mass e-mails. If 'No' is chosen, then the site's users will not be able to opt-out of receiving mass e-mails. If 'Yes' is chosen, then the site's users will be able to opt-out of receiving mass e-mails, and they will not receive any from any category. If 'Selected categories' is chosen, then the site's users will be able to opt-out of receiving mass e-mails from which ever categories they choose."),
  );

  $form['mass_contact_optout_options']['mass_contact_optout_message'] = array(
    '#type' => 'textarea',
    '#title' => t('The message to display to users when giving them the option to opt out'),
    '#default_value' => variable_get('mass_contact_optout_message', t('Allows you to opt-out of receiving mass e-mails from privileged users. Note that site administrators are able to include you in mass e-mails even if you choose not to enable this feature, and the ability to opt-out may be removed by the administrator at any time.')),
    '#description' => t('This is the message users will see in thier account settings page when they are presented with a list of categories to opt out of.'),
  );

  ////////////////////////////////////////////////////////////
  // Node copy options.
  $form['mass_contact_nodecc_d'] = array(
    '#type' => 'checkbox',
    '#title' => t('Save a copy as a node by default.'),
    '#default_value' => variable_get('mass_contact_nodecc_d', 1),
  );
  $form['mass_contact_nodecc_d_override'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow sender to override node copy setting.'),
    '#default_value' => variable_get('mass_contact_nodecc_d_override', 1),
  );

  ////////////////////////////////////////////////////////////
  // Flood control options.
  $form['mass_contact_hourly_threshold'] = array(
//    '#type' => 'select',
    '#type' => 'textfield',
    '#size' => 10,
    '#title' => t('Hourly threshold'),
//    '#options' => drupal_map_assoc(array(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 40, 50, 75, 100)),
    '#default_value' => variable_get('mass_contact_hourly_threshold', 3),
    '#description' => t('The maximum number of Mass Contact form submissions a user can perform per hour.'),
  );

  return system_settings_form($form);
} // End of mass_contact_admin_settings_misc().


/**
 * Message header administration settings form.
 *
 * @param form_state
 *   A keyed array containing the current state of the form.
 *
 * @return
 *   An associative array that defines the form to be built.
 */
function mass_contact_admin_settings_header($form, $form_state) {
  ////////////////////////////////////////////////////////////
  // The default character set.
  $form['mass_contact_character_set'] = array(
    '#type' => 'textfield',
    '#title' => t('Character set'),
    '#default_value' => variable_get('mass_contact_character_set', ''),
    '#description' => t('You may specify an alternate character set to use when sending e-mails. If left blank, the default of UTF-8 will be used. If you are unsure of what to put here, then leave it blank. Caution: setting this may not get you the results you desire. Other modules may come along and change that value after it has been set by this module.'),
  );

  ////////////////////////////////////////////////////////////
  // The sender's name and e-mail address.
  $form['mass_contact_default_sender'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default sender information'),
    '#description' => t('If anything is specified in here, it is used in place of the "Your name" and "Your e-mail address" fileds when sending the mass e-mail. Otherwise, the sender\'s name and e-mail address will be the default values. You must fill in both values, if you want to specify a default.'),
  );
  $form['mass_contact_default_sender']['mass_contact_default_sender_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Default sender name'),
    '#default_value' => variable_get('mass_contact_default_sender_name', ''),
    '#size' => 60,
    '#maxlength' => 128,
    '#description' => t('The optional user name to send e-mail as. Replaces the "Your name" value when sending mass e-mails.'),
  );
  $form['mass_contact_default_sender']['mass_contact_default_sender_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Default sender e-mail address'),
    '#default_value' => variable_get('mass_contact_default_sender_email', ''),
    '#size' => 60,
    '#maxlength' => 128,
    '#description' => t('The optional user e-mail address to send e-mail as. Replaces the "Your e-mail address" value when sending mass e-mails.'),
  );
  $form['mass_contact_default_sender']['mass_contact_default_sender_changable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow the sender to change these values.'),
    '#default_value' => variable_get('mass_contact_default_sender_changable', 0),
    '#description' => t('If checked, gives the sender the ability of changing the default sender and e-mail address when creating the message. If unchecked, the fields will be disabled.'),
  );

  ////////////////////////////////////////////////////////////
  // BCC options.
  $form['mass_contact_bcc_d'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send as BCC (hide recipients) by default.'),
    '#default_value' => variable_get('mass_contact_bcc_d', 1),
  );
  $form['mass_contact_bcc_d_override'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow sender to override BCC setting.'),
    '#default_value' => variable_get('mass_contact_bcc_d_override', 1),
  );

  ////////////////////////////////////////////////////////////
  // Category options.
  $form['mass_contact_category_override'] = array(
    '#type' => 'checkbox',
    '#title' => t('Include category in subject line.'),
    '#default_value' => variable_get('mass_contact_category_override', 1),
    '#description' => t("If you choose this option, the category name will be printed in square brackets preceeding the message sender's subject.
                        If the message sender has multiple categories selected with this option choosen, each category will be processed separately.
                        If you do not choose this option and the message sender has multiple categories selected, all users will be grouped to gether and the message will be sent to everyone as one group, thus reducing the likelihood of sending duplicates."),
  );

  return system_settings_form($form);
} // End of mass_contact_admin_settings_header().

/**
 * Validates the message header administration settings form.
 *
 * @param form
 *   An associative array containing the structure of the form.
 * @param form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_admin_settings_header_validate($form, &$form_state) {
  if (!empty($form_state['values']['mass_contact_default_sender_name'])) {
    if (empty($form_state['values']['mass_contact_default_sender_email'])) {
      form_set_error('mass_contact_default_sender_email', t('If you are going to specify default user settings, you must specify both a user name and a user e-mail address.'));
    }
  }

  if (!empty($form_state['values']['mass_contact_default_sender_email'])) {
    if (empty($form_state['values']['mass_contact_default_sender_name'])) {
      form_set_error('mass_contact_default_sender_name', t('If you are going to specify default user settings, you must specify both a user name and a user e-mail address.'));
    }
  }
} // End of mass_contact_admin_settings_header_validate().


/**
 * Message body administration settings form.
 *
 * @param form_state
 *   A keyed array containing the current state of the form.
 *
 * @return
 *   An associative array that defines the form to be built.
 */
function mass_contact_admin_settings_body($form, $form_state) {
  $mimemail = module_exists('mimemail');
  $token = module_exists('token');

  ////////////////////////////////////////////////////////////
  // Supplemental texts that are prepended and/or appended to every message.
  $form['mass_contact_supplemental_texts'] = array(
    '#type' => 'fieldset',
    '#title' => t('Supplemental message body texts'),
    '#description' => t('You may specify additional text to insert before and/or after the message text of every mass e-mail that is sent.'),
  );

  $mass_contact_message_prefix = variable_get('mass_contact_message_prefix');
  $mass_contact_message_suffix = variable_get('mass_contact_message_suffix');

  if ($mimemail) {
    $field_type = 'text_format';
    $prefix_format = isset($mass_contact_message_prefix['format']) ? $mass_contact_message_prefix['format'] : NULL;
    $suffix_format = isset($mass_contact_message_suffix['format']) ? $mass_contact_message_suffix['format'] : NULL;

    if ($token) {
      $prefix_default_value = isset($mass_contact_message_prefix['value']) ? $mass_contact_message_prefix['value'] : t('[current-user:name] has sent you a group e-mail from [site:name].');
      $suffix_default_value = isset($mass_contact_message_suffix['value']) ? $mass_contact_message_suffix['value'] : '';
    }
    else {
      $prefix_default_value = isset($mass_contact_message_prefix['value']) ? $mass_contact_message_prefix['value'] : t('You were sent a group e-mail from @site.', array('@site' => url(NULL, array('absolute' => TRUE))));
      $suffix_default_value = isset($mass_contact_message_suffix['value']) ? $mass_contact_message_suffix['value'] : '';
    }
  }
  else {
    $field_type = 'textarea';
    $prefix_format = NULL;
    $suffix_format = NULL;

    if ($token) {
      $prefix_default_value = isset($mass_contact_message_prefix) ? $mass_contact_message_prefix : t('[current-user:name] has sent you a group e-mail from [site:name].');
      $suffix_default_value = isset($mass_contact_message_suffix) ? $mass_contact_message_suffix : '';
    }
    else {
      $prefix_default_value = isset($mass_contact_message_prefix) ? $mass_contact_message_prefix : t('You were sent a group e-mail from @site.', array('@site' => url(NULL, array('absolute' => TRUE))));
      $suffix_default_value = isset($mass_contact_message_suffix) ? $mass_contact_message_suffix : '';
    }
  }

  $form['mass_contact_supplemental_texts']['mass_contact_message_prefix'] = array(
    '#type' => $field_type,
    '#title' => t('Text to be prepended to all messages'),
    '#default_value' => $prefix_default_value,
    '#format' => $prefix_format,
    '#description' => t('The text you specify in this field will be added to all Mass Contact messages sent out and will be placed before the message text entered in by the sender.'),
  );

  $form['mass_contact_supplemental_texts']['mass_contact_message_suffix'] = array(
    '#type' => $field_type,
    '#title' => t('Text to be appended to all messages'),
    '#default_value' => $suffix_default_value,
    '#format' => $suffix_format,
    '#description' => t('The text you specify in this field will be added to all Mass Contact messages sent out and will be placed after the message text entered in by the sender.'),
  );

  if ($token) {
    // Display the user documentation of placeholders supported by this
    // module, as a description on the last pattern
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

  ////////////////////////////////////////////////////////////
  // HTML options.
  $form['mass_contact_html_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('HTML Settings'),
  );
  if ($mimemail) {
    $mass_contact_html_format = variable_get('mass_contact_html_format');
    $form['mass_contact_html_settings']['mass_contact_html_format'] = array(
      '#type' => 'text_format',
      '#base_type' => 'item',
      '#title' => t('The default text format'),
      '#format' => isset($mass_contact_html_format['format']) ? $mass_contact_html_format['format'] : NULL,
      '#description' => t('This is the text format that will be initially selected. If you do not want to allow HTML messages, then specify a plain text text format and do not aloow it to be overridden below. Keep in mind that the user sending the message may not have access to all the text formats that are available here.'),
    );

    $form['mass_contact_html_settings']['mass_contact_html_format_override'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow sender to override the text format.'),
      '#default_value' => variable_get('mass_contact_html_format_override', 0),
    );

//    $form['mass_contact_html_settings']['mass_contact_check_markup'] = array(
//      '#type' => 'checkbox',
//      '#title' => t('Check HTML markup.'),
//      '#default_value' => variable_get('mass_contact_check_markup', 1),
//      '#description' => t('Runs the body text through an HTML markup checker. Disabling this is not recommeneded. However, if your messages are not coming through as they should, this may fix it.'),
//    );
  }
  else {
    $form['mass_contact_html_settings']['mass_contact_no_mimemail'] = array(
      '#type' => 'item',
      '#description' => t('This module no longer supports HTML e-mail without the Mime Mail module, which can be found here: http://drupal.org/project/mimemail.'),
    );
  }

  ////////////////////////////////////////////////////////////
  // Attachment options.
  $form['mass_contact_attachment_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Attachment Settings'),
  );
  if ($mimemail) {
    $form['mass_contact_attachment_settings']['mass_contact_number_of_attachments'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of attachments'),
      '#default_value' => variable_get('mass_contact_number_of_attachments', '3'),
      '#size' => 10,
      '#description' => t("The number of attachments to allow on the contact form. The maximum number of allowed uploads may be limited by PHP. If necessary, check your system's PHP php.ini file for a max_file_uploads directive to change."),
    );
    $form['mass_contact_attachment_settings']['mass_contact_attachment_location'] = array(
      '#type' => 'textfield',
      '#title' => t('Attachment location'),
      '#default_value' => variable_get('mass_contact_attachment_location', variable_get('file_public_path', conf_path() . '/files') . '/mass_contact_attachments'),
      '#description' => t('If a copy of the message is saved as a node, this is the file path where to save the attachment so it can be viewed later.'),
    );
  }
  else {
    $form['mass_contact_attachment_settings']['mass_contact_no_mimemail'] = array(
      '#type' => 'item',
      '#description' => t('This module no longer supports attachments without the Mime Mail module, which can be found here: http://drupal.org/project/mimemail.'),
    );
  }

  return system_settings_form($form);
} // End of mass_contact_admin_settings_body().
