<?php

/**
 * @file
 * This is the main code file for the Mass Contact module.
 *
 * This module enables users to send messages (contact) to multiple users
 * through selected roles.
 */

 /* ********************************
 *
 * Functions for sending a message.
 *
 * *******************************/

/**
 * The mail page
 *
 * @return
 *   Either an error, if flood control is active and triggered, or a
 *   rendered form.
 */
function mass_contact_site_page() {
  global $user;

  if (!user_access('administer mass contact') && !flood_is_allowed('mass_contact', variable_get('mass_contact_hourly_threshold', 3))) {
    $output = t("You cannot send more than %number messages per hour. Please try again later.", array('%number' => variable_get('mass_contact_hourly_threshold', 3)));
  }
  else {
    $output = drupal_get_form('mass_contact_mail_page');
  }

  return $output;
} // End of mass_contact_site_page().


/**
 * Generates the main Mass Contact mail form.
 *
 * @param form_state
 *   A keyed array containing the current state of the form.
 *
 * @return
 *   An associative array that defines the form to be built.
 */
function mass_contact_mail_page($form, $form_state) {
  global $user;

  $form = array();
  $categories = array();
  $default_category = array();
  $default_category_name = '';

//  $result = db_query("SELECT cid, category, selected FROM {mass_contact} ORDER BY weight, category");
  $result = db_select('mass_contact', 'mc')
    ->fields('mc', array('cid', 'category', 'selected'))
    ->orderBy('weight', 'ASC')
    ->orderBy('category', 'ASC')
    ->execute();
  foreach ($result as $category) {
    if (user_access('send to users in the ' . $category->category . ' category')) {
      $categories[$category->cid] = $category->category;

      if ($category->selected) {
        $default_category[] = $category->cid;
        $default_category_name = $category->category;
      }
    }
  }

  if (count($categories) == 1) {
    $default_category = array_keys($categories);
    $default_category_name = $categories[$default_category[0]];
  }

  if (count($categories) > 0) {
    $form['#attributes'] = array(
      'enctype' => "multipart/form-data",
    );
    $form['#token'] = $user->name . $user->mail;
    $form['contact_information'] = array(
      '#value' => filter_xss_admin(variable_get('mass_contact_form_information', t('Send an e-mail message using the contact form below.'))),
    );

    ////////////////////////////////////////////////////////////
    // Add the field for specifying the sender's name.
    $mass_contact_default_sender_name = variable_get('mass_contact_default_sender_name', '');
    if ($mass_contact_default_sender_name) {
      if (variable_get('mass_contact_default_sender_changable', 0)) {
        $form['name'] = array(
          '#type' => 'textfield',
          '#title' => t('Your name'),
          '#maxlength' => 255,
          '#default_value' => $mass_contact_default_sender_name,
          '#required' => TRUE,
        );
      }
      else {
        $form['name'] = array(
          '#type' => 'item',
          '#title' => t('Your name'),
          '#value' => $mass_contact_default_sender_name,
        );
      }
    }
    else {
      $form['name'] = array(
        '#type' => 'textfield',
        '#title' => t('Your name'),
        '#maxlength' => 255,
        '#default_value' => $user->uid ? $user->name : '',
        '#required' => TRUE,
      );
    }

    ////////////////////////////////////////////////////////////
    // Add the field for specifying the sender's e-mail address.
    $mass_contact_default_sender_email = variable_get('mass_contact_default_sender_email', '');
    if ($mass_contact_default_sender_email) {
      if (variable_get('mass_contact_default_sender_changable', 0)) {
        $form['mail'] = array(
          '#type' => 'textfield',
          '#title' => t('Your e-mail address'),
          '#maxlength' => 255,
          '#default_value' => $mass_contact_default_sender_email,
          '#required' => TRUE,
        );
      }
      else {
        $form['mail'] = array(
          '#type' => 'item',
          '#title' => t('Your e-mail address'),
          '#value' => $mass_contact_default_sender_email,
        );
      }
    }
    else {
      $form['mail'] = array(
        '#type' => 'textfield',
        '#title' => t('Your e-mail address'),
        '#maxlength' => 255,
        '#default_value' => $user->uid ? $user->mail : '',
        '#required' => TRUE,
      );
    }

    ////////////////////////////////////////////////////////////
    // Add the field for specifying the category(ies).
    if ((count($categories) > 1) || !isset($default_category)) {
      // Display a choice when one is needed.
      $form['cid'] = array(
        '#type' => 'select',
        '#title' => t('Category'),
        '#default_value' => $default_category,
        '#options' => $categories,
        '#required' => TRUE,
        '#multiple' => TRUE,
      );
    }
    else {
      // Otherwise, just use the default category.
      $form['cid'] = array(
        '#type' => 'value',
        '#value' => $default_category, // This must be an array, otherwise the code breaks upon submit.
      );
      $form['cid-info'] = array(
        '#type' => 'markup',
        '#value' => '<p>Sending to all users subscribed to the ' . $default_category_name . ' category.</p>',
      );
    }

    ////////////////////////////////////////////////////////////
    // Add the field for specifying whether opt-outs are respected or not.
    $optout_setting = variable_get('mass_contact_optout_d', 0);

    // Allow users to opt-out of mass e-mails:
    // 0 => 'No', 1 == 'Yes', 2 == 'Selected categories'.
    if ($optout_setting == 1 || $optout_setting == 2) {
      // Allow to override or respect opt-outs if admin, otherwise use default.
      if (user_access('administer mass contact')) {
        $form['optout'] = array(
          '#type' => 'checkbox',
          '#title' => t('Respect user opt-outs.'),
          '#default_value' => 1,
        );
      }
      else {
        $form['optout'] = array(
          '#type' => 'hidden',
          '#default_value' => 1,
        );
      }
    }
    else {
      $form['optout'] = array(
        '#type' => 'hidden',
        '#default_value' => 0,
      );
    }

    ////////////////////////////////////////////////////////////
    // Add the field for specifying whether the recipients are in the To or
    // BCC field of the message.
    // Check if the user can override the BCC setting.
    if (variable_get('mass_contact_bcc_d_override', 1)) {
      $form['bcc'] = array(
        '#type' => 'checkbox',
        '#title' => t('Send as BCC (hide recipients).'),
        '#default_value' => variable_get('mass_contact_bcc_d', 1),
      );
    }
    // If not, then just display the BCC info.
    else {
      $form['bcc'] = array(
        '#type' => 'value',
        '#value' => variable_get('mass_contact_bcc_d', 1),
      );
      $form['bcc-info'] = array(
        '#type' => 'markup',
        '#value' => '<p>' . (variable_get('mass_contact_bcc_d', 1) ? t('Recipients will be hidden.') : t('Recipients will NOT be hidden.')) . '</p>',
      );
    }

    ////////////////////////////////////////////////////////////
    // Add the field for specifying the subject of the message.
    $form['subject'] = array(
      '#type' => 'textfield',
      '#title' => t('Subject'),
      '#maxlength' => 255,
      '#required' => TRUE,
    );

    ////////////////////////////////////////////////////////////
    // Add the field for specifying the body and text format of the message.
    //
    // Get the HTML input format setting and the corresponding name.
    if (module_exists('mimemail')) {
      // Get the full object of the admin specified default text format.

      // Get the admin specified default text format.
      $html_filter_format = variable_get('mass_contact_html_format', filter_fallback_format());

      // Initialize this variable.
      $default_filter_format = NULL;

      // Get all the text formats the user is allowed access to.
      $filter_formats = filter_formats($user);

      // Cycle through the list of possible text formats.
      foreach ($filter_formats as $filter_format) {
        // If the current text format matches the admin specified one...
        if ($filter_format->format == $html_filter_format['format']) {
          // ...save it for later use.
          $default_filter_format = $filter_format;
        }
      }

      // Check if the user is allowed to override the text format.
      if (variable_get('mass_contact_html_format_override', 0)) {
        // The user is allowed to override the text format, so display a
        // textarea that has the text format selection field attached.
        $form['body_filter']['message'] = array(
          '#type' => 'text_format',
          '#title' => t('Message'),
          '#format' => isset($default_filter_format->format) ? $default_filter_format->format : NULL,
          '#rows' => 12,
          '#required' => TRUE,
        );
      }
      else {
        // The user is not allowed to override the text format, so display a
        // textarea that does not have the text format selection field
        // attached.
        $form['body_filter']['message'] = array(
          '#type' => 'textarea',
          '#title' => t('Message'),
          '#rows' => 12,
          '#required' => TRUE,
        );
        // Save the text format information for later use.
        $form['html_format'] = array(
          '#type' => 'value',
          '#value' => $default_filter_format,
        );
        // Display text format information to the user.
        $form['html_info'] = array(
          '#type' => 'item',
          '#description' => '<p>' . (variable_get('mass_contact_html_d', 1) ? t('The message will be sent using the %filter text format. More information about what is available is on the <a href="@formats_descriptions">Compose tips</a> page.', array('%filter' => $default_filter_format->name, '@formats_descriptions' => url('filter/tips'))) : t('The message will be sent as plain text.')) . '</p>',
        );
      }

      ////////////////////////////////////////////////////////////
      // If the user has access, add the field for specifying the attachment.
      if (user_access('send mass contact attachments')) {
        for ($i = 1; $i <= variable_get('mass_contact_number_of_attachments', '3'); $i++) {
          $form['attachment_' . $i] = array(
            '#type' => 'file',
            '#title' => t('Attachment #!number', array('!number' => $i)),
          );
        }
      }
    }
    else {
      // Mime Mail is not installed, so display a textarea that does not have
      // the text format selection field attached, as this message will only
      // be sent as plain text.
      $form['body_filter']['message'] = array(
        '#type' => 'textarea',
        '#title' => t('Message'),
        '#rows' => 12,
        '#required' => TRUE,
      );
      // Display text format information to the user.
      $form['html_info'] = array(
        '#type' => 'item',
        '#description' => '<p>' . t('The message will be sent as plain text.') . '</p>',
      );
    }

    ////////////////////////////////////////////////////////////
    // We do not allow anonymous users to send themselves a copy because it
    // can be abused to spam people.
    if ($user->uid) {
      $form['copy'] = array(
        '#type' => 'checkbox',
        '#title' => t('Send yourself a copy.'),
      );
    }

    ////////////////////////////////////////////////////////////
    // Add the field for specifying whether to save the message as a node or
    // not.
    if (user_access('choose whether to archive mass contact messages')) {
      // Check if the user can override the node copy setting.
      if (variable_get('mass_contact_nodecc_d_override', 1)) {
        $form['nodecc'] = array(
          '#type' => 'checkbox',
          '#title' => t('Save a copy as a node.'),
          '#default_value' => variable_get('mass_contact_nodecc_d', 1),
        );
      }
      // If not, then do it or not based on the administrative setting.
      else {
        $form['nodecc'] = array(
          '#type' => 'hidden',
          '#default_value' => variable_get('mass_contact_nodecc_d', 1),
        );
      }
    }
    // If not, then do it or not based on the administrative setting.
    else {
      $form['nodecc'] = array(
        '#type' => 'hidden',
        '#default_value' => variable_get('mass_contact_nodecc_d', 1),
      );
    }

    /*
     // Place holder for future use.
     $form['preview'] = array(
     '#type' => 'button',
     '#value' => t('Preview')
     );
     */

    ////////////////////////////////////////////////////////////
    // Add the submit button.
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Send e-mail'),
    );
  }
  else {
    $form['error'] = array(
      '#markup' => '<p><b>' . t('Either you have not created any categories, or you are not allowed to send to any of the existing categories.') . '<br /><br />' . t('Either create at least one category of users to send to, or contact your system administer for access to the existing categories.') . '</b>',
    );
  }

  if (user_access('administer mass contact')) {
    $form['tasklist'] = array(
      '#type' => 'fieldset',
      '#title' => t('Related tasks'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#prefix' => '<p>',
    );
    $form['tasklist']['list'] = array(
      '#markup' => '<p><ol>' .
        '<li>' . l(t('Set Permissions'), 'admin/user/permissions', array('fragment' => 'module-mass_contact')) . '</li>' .
        '<li>' . l(t('List current categories'), 'admin/config/system/mass_contact') . '</li>' .
        '<li>' . l(t('Add new category'), 'admin/config/system/mass_contact/add') . '</li>' .
        '<li>' . l(t('Configure the module'), 'admin/config/system/mass_contact/settings') . '</li>' .
        '<li>' . l(t('Help'), 'admin/help/mass_contact') . '</li></ol>',
    );
  }

  return $form;
} // End of mass_contact_mail_page().


/**
 * Validates the main Mass Contact mail form.
 *
 * @param form
 *   An associative array containing the structure of the form.
 * @param form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_mail_page_validate($form, &$form_state) {
  if (!$form_state['values']['cid']) {
    form_set_error('category', t('You must select a valid category.'));
  }

  if (variable_get('mass_contact_default_sender_changable', 0) == 1 && !valid_email_address($form_state['values']['mail'])) {
    form_set_error('mass_contact', t('You must enter a valid e-mail address.'));
  }
} // End of mass_contact_mail_page_validate().


/**
 * Processes the main Mass Contact mail form.
 *
 * @param form
 *   An associative array containing the structure of the form.
 * @param form_state
 *   A keyed array containing the current state of the form.
 */
function mass_contact_mail_page_submit($form, &$form_state) {
  // Initialize some variables.
  $params = array();
  $bcc = $form_state['values']['bcc'];
  $nodecc = $form_state['values']['nodecc'];
  $message = '';
  $send_error = 0;

  $default_sender_name = variable_get('mass_contact_default_sender_name');
  if (!empty($form_state['values']['name'])) {
    $from_name = $form_state['values']['name'];
  }
  elseif (!empty($default_sender_name)) {
    $from_name = $default_sender_name;
  }
  else {
    $from_name = variable_get('site_name');
  }

  $default_sender_email = variable_get('mass_contact_default_sender_email');
  if (!empty($form_state['values']['mail'])) {
    $from_email = $form_state['values']['mail'];
  }
  elseif (!empty($default_sender_email)) {
    $from_email = $default_sender_email;
  }
  else {
    $from_email = variable_get('site_mail');
  }

  // Check for the exsistance of attachments.
  $attachments = FALSE;
  if (module_exists('mimemail')) {
    for ($i = 1; $i <= variable_get('mass_contact_number_of_attachments', '3'); $i++) {
      if (isset($_FILES['files']['size']['attachment_' . $i]) && $_FILES['files']['size']['attachment_' . $i] > 0) {
        $attachments = TRUE;
        break;
      }
    }
  }


  // Prepare the message header. This currently only sets the character set.
  $params = _mass_contact_prepare_header();

  // Prepare the message body parts.
  $message = _mass_contact_prepare_bodies($form_state['values']);

  // For the node copy, we want to handle attachments differently, so we'll
  // save the message as it currently stands.
  if (variable_get('mass_contact_nodecc_d', 1) || ($nodecc == 1)) {
    $node_message = $message;
  }

  // If there are attachments, add them.
  if ($attachments) {
    $params['attachments'] = _mass_contact_process_mime_mail_attachments();
  }


  // Add the message to the main variable.
  $params['body'] = $message;


  // Load the category information.
  $cids = array();
  foreach ($form_state['values']['cid'] as $cid) {
    $cids[] = $cid;
  }

  // Select the chosen categories from the database.
  $categories = db_select('mass_contact', 'mc')
    ->condition('cid', $cids, 'IN')
    ->fields('mc')
    ->execute();

  // Check to see of the authenticated users role was specified in any
  // category.
  $everyone = FALSE;
  $message_subject = '';

  foreach ($categories as $category) {
    // Get the list of roles which were chosen through the categry selection.
    $roles = explode(',', $category->recipients);
    foreach ($roles as $role) {
      // Role 2 is the authenticated users role, which means everyone. This
      // needs to be checked for first. Otherwise, duplicate messages could be
      // sent.
      if ($role == 2) {
        $everyone = TRUE;

        if (variable_get('mass_contact_category_override', 1)) {
          $message_subject = t('[@category] @subject', array('@category' => $category->category, '@subject' => $form_state['values']['subject']));
        }

        break 2;
      }
    }
  }

  // Get the list of users who've opted out of this mailing.
  $opt_out_users = array();

  if ($form_state['values']['optout']) {
    // Get the list of users who've opted out.
    $opt_out_users = _mass_contact_create_opt_out_list($categories);
  }


  // Create the recipient lists.

  // Initialize the array of recipients and subjects.
  $recipient_lists = array();

  // If the 'authentecated users' role was selected in any category, we ignore
  // all categories.
  if ($everyone) {
    // Set subject.
    if ($message_subject == '') {
      $message_subject = $form_state['values']['subject'];
    }

    // Create the recipient list, consisting of all users.
    $recipient_lists['All authenticated users'] = array(
      // Set the recipients.
      'recipients' => _mass_contact_create_recipient_list(array(2), $opt_out_users),
      // Set the subject.
      'subject' => $message_subject,
    );
  }
  // If the module is configured to not include the category in the subject,
  // combine all users in all categories/roles into a single listing of
  // recpients.
  elseif (!variable_get('mass_contact_category_override', 1)) {
    $roles = array();

    // Select the chosen categories from the database.
    $categories = db_select('mass_contact', 'mc')
      ->condition('cid', $cids, 'IN')
      ->fields('mc')
      ->execute();

    // Cycle through all the choosen categories.
    foreach ($categories as $category) {
      // Get the list of roles which were chosen through the categry selection.
      $roles = array_merge($roles, explode(',', $category->recipients));
    }

    // Create the recipient list.
    $recipient_lists['All categories combined'] = array(
      // Set the recipients.
      'recipients' => _mass_contact_create_recipient_list($roles, $opt_out_users),
      // Set the subject.
      'subject' => $form_state['values']['subject'],
    );
  }
  // Cycle through all categories and create groups of recipients.
  else {
    // Select the chosen categories from the database.
    $categories = db_select('mass_contact', 'mc')
      ->condition('cid', $cids, 'IN')
      ->fields('mc')
      ->execute();
    foreach ($categories as $category) {
      // Get the list of roles which were chosen through the categry selection.
      $roles = explode(',', $category->recipients);

      // Create the recipient lists, per category.
      $recipient_lists[$category->category] = array(
        // Set the recipients.
        'recipients' => _mass_contact_create_recipient_list($roles, $opt_out_users),
        // Set the subject.
        'subject' => t('[@category] @subject', array('@category' => $category->category, '@subject' => $form_state['values']['subject'])),
      );
    }
  }


  // Verify we have someone to send to.
  $no_recipients = FALSE;
  $recipients_count = 0;

  // Iterate through each recipient list to verify there are recipients
  // somewhere.
  if (!empty($recipient_lists)) {
    foreach ($recipient_lists as $recipient_list) {
      // Check for empty recipient list.
      if (count($recipient_list['recipients']) == 0) {
        $recipients_count++;
      }
    }
  }

  // There are categories selected, but all of them are empty.
  if (count($recipient_lists) == $recipients_count) {
    drupal_set_message(t('No users were in the selected category(ies), and therefore, the message was not sent.'));
    $no_recipients = TRUE;
    $form_state['redirect'] = '/';
  }

  if (!$no_recipients) {
    // We have recipients, so send the message to them.

//    global $user;
    $total_recipients = 0;
    $all_recipients = array();

    // Iterate through each recipient list.
    foreach ($recipient_lists as $category => $recipient_list) {
      // Only process this recipient list if it is not empty.
      if (count($recipient_list['recipients']) > 0) {
        // If there's a recipient limit, break up recipient list.
        $recipient_limit = variable_get('mass_contact_recipient_limit', 0);
        if (count($recipient_list) > $recipient_limit && $recipient_limit != 0) {
          $recipient_count = 0;
          $ccc = 0;

          foreach ($recipient_list as $recipient_name => $recipient_address) {
            // $all_recipients[] = $recipient_name . " <" . $recipient_address . ">";
            // $partial_list[] = $recipient_name . " <" . $recipient_address . ">";
            $all_recipients[] = $recipient_address;
            $partial_list[] = $recipient_address;
            $recipient_count = count($partial_list);

            if ($recipient_count == $recipient_limit) {
              $recipients = implode(', ', $partial_list);

              // If blind carbon copy (BCC) was selected, put the recipients in
              // the Bcc field.
              if ($bcc == 1) {
                $params['headers']['Bcc'] = $recipients;
                $to = $from_email;
              }
              // Else, put the recipients in the To field.
              else {
                $to = $recipients;
              }

              $params['subject'] = $recipient_list['subject'];

              ++$ccc;

              $success = drupal_mail('mass_contact', 'mail_page', $to, language_default(), $params, $from_email);

              if ($success['result']) {
                drupal_set_message(t('[Success] Send #@ccc: @e-mails', array('@ccc' => $ccc, '@e-mails' => $recipients)));
              }
              else {
                ++$send_error;
              }

              if ($nodecc == 1) {
                // Save the message as a node.
                _mass_contact_save_node($params, $node_message, $from_email, $category, $to, $boundary_attachment, $success['result']);
              }

              // Reset the variables.
              $partial_list = array();
              $recipient_count = 0;
            }
          }

          // Send the remainder.
          if ($recipient_count != 0) {
            $recipients = implode(', ', $partial_list);

            // If blind carbon copy (BCC) was selected, put the recipients in
            // the Bcc field.
            if ($bcc == 1) {
              $params['headers']['Bcc'] = $recipients;
              $to = $from_email;
            }
            else {
              // Else, put the recipients in the To field.
              $to = $recipients;
            }

            $params['subject'] = $recipient_list['subject'];

            ++$ccc;

            $success = drupal_mail('mass_contact', 'mail_page', $to, language_default(), $params, $from_email);

            if ($success['result']) {
              drupal_set_message(t('[Success] Send Remainder: @e-mails', array('@e-mails' => $recipients)));
            }
            else {
              ++$send_error;
            }

            if ($nodecc == 1) {
              // Save the message as a node.
              _mass_contact_save_node($params, $node_message, $from_email, $category, $to, $boundary_attachment, $success['result']);
            }
          }
          $total_recipients = count($all_recipients);
          $listed_recipients = implode(', ', $all_recipients);
        }
        // There's no recipient limit, so send the message to all the recipients
        // at the same time.
        else {
          foreach ($recipient_list as $recipient_name => $recipient_address) {
            //          $all_recipients[] = $recipient_name . " <" . $recipient_address . ">";
            $all_recipients[] = $recipient_address;
          }

          $total_recipients = count($all_recipients);
          $listed_recipients = implode(', ', $all_recipients[0]);

          // If blind carbon copy (BCC) was selected, put the recipients in
          // the Bcc field.
          if ($bcc == 1) {
            $params['headers']['Bcc'] = $listed_recipients;
            $to = $from_email;
          }
          else {
            // Else, put the recipients in the To field.
            $to = $listed_recipients;
          }

          $params['subject'] = $recipient_list['subject'];

          $success = drupal_mail('mass_contact', 'mail_page', $to, language_default(), $params, $from_email);

          if ($success['result']) {
            drupal_set_message(t('[Success] Send Once: @e-mails', array('@e-mails' => $listed_recipients)));
          }
          else {
            ++$send_error;
          }

          if ($nodecc == 1) {
            // Save the message as a node.
            _mass_contact_save_node($params, $node_message, $from_email, $category, $to, $boundary_attachment, $success['result']);
          }
        }
      }
    }
  }

  // Send a copy to self, if requested.
  if (!$no_recipients && $form_state['values']['copy'] && !$bcc) {
    $success = drupal_mail('mass_contact', 'user-copy', $to, language_default(), $params, $from_email);

    // Inform the user.
    if ($success['result']) {
      drupal_set_message(t('A copy was sent to you.'));
    }
    else {
      if (!module_exists('help')) {
        drupal_set_message(t("A copy could not be sent to you. Check the logs for error messages. For more information, see the Troubleshooting secion of Mass Contact's !help page or README.txt.", array('@errors' => $send_error, '!help' => l(t('help'), 'admin/help/mass_contact'))));
      }
      else {
        drupal_set_message(t("A copy could not be sent to you. Check the logs for error messages. For more information, see the Troubleshooting secion of Mass Contact's README.txt.", array('@errors' => $send_error)));
      }
    }
  }

  // Final tasks.
  if (!$no_recipients) {
    // Check for errors.
    if ($send_error == 0) {
      // If there are no errors, check to see if we need to save the message as a node.
//      if ($nodecc == 1) {
        // Save the message as a node.
//        _mass_contact_save_node($params['subject'], $node_message, $listed_recipients, $category->category, $roles, $user->uid, $boundary_attachment);
//        _mass_contact_save_node($params, $node_message, $from_email, $categories, $roles, $listed_recipients, $boundary_attachment);
//      }

      // Log the operation.
      flood_register_event('mass_contact');
      watchdog('mass_contact', '%name-from sent an e-mail to the %category group.', array('%name-from' => $from_name . ' <' . $from_email . '>', '%category' => $category));

      // Inform the user.
      if (count($total_recipients) > 0) {
        drupal_set_message(t('Message sent successfully to @total users: @e-mails', array('@total' => $total_recipients, '@e-mails' => $listed_recipients)));
      }
    }
    else {
      // Inform the user of the error.
      if (module_exists('help')) {
        drupal_set_message(t("@errors error(s) were encountered sending the message. Check the logs for error messages. For more information, see the Troubleshooting section of Mass Contact's !help page or README.txt.", array('@errors' => $send_error, '!help' => l(t('help'), 'admin/help/mass_contact'))));
      }
      else {
        drupal_set_message(t("@errors error(s) were encountered sending the message. Check the logs for error messages. For more information, see the Troubleshooting section of Mass Contact's README.txt.", array('@errors' => $send_error)));
      }
    }
  }

  // Redirect to the home page, rather than back to the mail page, to avoid
  // contradictory messages if flood control has been activated.
  $form_state['redirect'] = '/';
} // End of mass_contact_mail_page_submit().


/* *************************
 *
 * Private helper functions.
 *
 * ************************/


/**
 * Prepares the message's header.
 *
 * @param boundary_attachment
 *   A string containing boundary for the attachments.
 * @param boundary_html
 *   A string containing boundary for the HTML part.
 *
 * @return
 *   An array containing the prepared header.
 */
//function _mass_contact_prepare_header($boundary_attachment, $boundary_html) {
function _mass_contact_prepare_header() {
  $params = array();

  if (module_exists('mimemail')) {
//    $params['plain'] = !$email['html'];
//    $params['plaintext'] = $email['html'] ? NULL : $email['message'];
    // Mime Mail requires this header or it will filter all text.
    $params['headers']['Content-Type'] = 'text/html; charset=UTF-8';
  }
  elseif ($character_set = variable_get('mass_contact_character_set', '')) {
    $params['headers']['Content-Type'] = 'text/plain; charset=' . $character_set . '; format=flowed';
  }

  return $params;
} // End of _mass_contact_prepare_header().


/**
 * Prepares the message's body.
 *
 * @param form_values
 *   A keyed array containing the values returned with the form.
 *
 * @return
 *   A keyed array containing the prepared body as two strings. There are two
 *   possible values:
 *     $body['plain']: The plain text body part is here.
 *     $body['html']: The HTML body part is here.
 */
function _mass_contact_prepare_bodies($form_values) {
//  $body = array('plain' => '', 'html' => '');
  $body = '';
//  $check_markup = variable_get('mass_contact_check_markup', 1);

  if (module_exists('mimemail')) {
/*
    // Create the plain text body part.

    // Start with the message prefix.
    $body['plain'] .= _mass_contact_prepare_message_addon('prefix', 'converted');

    // Add in the actual message.
    if ($check_markup) {
      $body['plain'] .= drupal_html_to_text(check_markup($form_values['message']['value'], $form_values['message']['format'])) . "\n";
    }
    else {
      $body['plain'] .= drupal_html_to_text($form_values['message']['value']) . "\n";
    }

    // End with the message suffix.
    $body['plain'] .= _mass_contact_prepare_message_addon('suffix', 'converted');
*/
    // Create the HTML body part.

    // Start with the message prefix.
    $body .= _mass_contact_prepare_message_addon('prefix', 'html');

    // Add in the actual message.
//    if ($check_markup) {
//      $body .= check_markup($form_values['message']['value'], $form_values['message']['format']);
//    }
//    else {
      $body .= $form_values['message']['value'];
//    }

    // End with the message suffix.
    $body .= _mass_contact_prepare_message_addon('suffix', 'html');
  }
  // Mime mail is not installed, so this message will only be processed as
  // plain text.
  else {
    // Start with the message prefix.
    $body .= _mass_contact_prepare_message_addon('prefix', 'converted');

    // Add in the actual message.
    $body .= wordwrap(drupal_html_to_text($form_values['message']['value'])) . "\n";

    // End with the message suffix.
    $body .= _mass_contact_prepare_message_addon('suffix', 'converted');
  }

  return $body;
} // End of _mass_contact_prepare_bodies().


/**
 * Prepares a message addon (prefix or suffix).
 *
 * @param add_on
 *   A string specifing the addon part to process. There are two possible
 *   values:
 *     'prefix'
 *     'suffix'
 * @param style
 *   A string indicating how to process the addon. There are three possible
 *   values:
 *     'plain text': Indicates that the returned addon should be in plain
 *       text format.
 *     'html': Indicates that the returned addon should be in HTML format.
 *     'converted': Indicates that the returned addon is HTML but should be
 *       converted into plain text format.
 *
 * @return
 *   A string containing the prepared message addon.
 */
function _mass_contact_prepare_message_addon($addon, $style) {
  $token_module_exists = module_exists('token');
  $message = '';
//  $check_markup = variable_get('mass_contact_check_markup', 1);

  switch ($addon) {
    case 'prefix':
      $message_addon = variable_get('mass_contact_message_prefix', '');
//      $addon_format = variable_get('mass_contact_prefix_format', filter_fallback_format());
      break;
    case 'suffix':
      $message_addon = variable_get('mass_contact_message_suffix', '');
//      $addon_format = variable_get('mass_contact_suffix_format', filter_fallback_format());
      break;
  }

  if (!empty($message_addon['value'])) {
    $message_addon = $message_addon['value'];

    switch ($style) {
      case 'plain text':
        if ($token_module_exists) {
          $message .= wordwrap(token_replace($message_addon)) . "\n";
        }
        else {
          $message .= wordwrap($message_addon) . "\n";
        }
        break;

      case 'html':
        if ($token_module_exists) {
//          if ($check_markup) {
//            $message .= check_markup(token_replace($message_addon), $addon_format);
//          }
//          else {
            $message .= token_replace($message_addon);
//          }
        }
//        elseif ($check_markup) {
//          $message .= check_markup($message_addon, $addon_format);
//        }
        else {
          $message .= $message_addon;
        }
        break;

      case 'converted':
        if ($token_module_exists) {
//          if ($check_markup) {
//            $message .= drupal_html_to_text(check_markup(token_replace($message_addon), $addon_format)) . "\n";
//          }
//          else {
            $message .= drupal_html_to_text(token_replace($message_addon)) . "\n";
//          }
        }
//        elseif ($check_markup) {
//          $message .= drupal_html_to_text(check_markup($message_addon, $addon_format)) . "\n";
//        }
        else {
          $message .= drupal_html_to_text($message_addon) . "\n";
        }
        break;
    }
  }

  return $message;
} // End of _mass_contact_prepare_message_addon().


/**
 * Process attachments for use with Mime Mail.
 *
 * @return
 *   The attachment information, as Mime Mail expects it.
 */
function _mass_contact_process_mime_mail_attachments() {
  // Set an initial value.
  $files = array();

  // Loop through each possible attachment.
  for ($i = 1; $i <= variable_get('mass_contact_number_of_attachments', '3'); $i++) {
    // Check to see if the attachment exists.
    if ($_FILES['files']['size']['attachment_' . $i] > 0) {
      $files[] = array(
        'filepath' => $_FILES['files']['tmp_name']['attachment_' . $i],
        'filename' => $_FILES['files']['name']['attachment_' . $i],
        'filemime' => $_FILES['files']['type']['attachment_' . $i],
      );
    }
  }
/*
         // Load attachments for the e-mail.
        $attachments = array();
        if ($email['attachments']) {
          webform_component_include('file');
          foreach ($node->webform['components'] as $component) {
            if (webform_component_feature($component['type'], 'attachment') && !empty($submissio$
              $file = webform_get_file($submission->data[$component['cid']]['value'][0]);
              if ($file && ($file->filepath = drupal_realpath($file->uri))) {
                $attachments[] = $file;
              }
            }
          }
        }

        // Add the attachments to the mail parameters.
        $mail_params['attachments'] = $attachments;

*/
  return $files;
} // End of _mass_contact_process_mime_mail_attachments().


/**
 * Creates a list of users who have opted out of receiving messages.
 *
 * @param categories
 *   An object containing the list of categories the message is to.
 *
 * @return
 *   An array with the keys being the user ids and the values being 1 to
 *   indicate the users' desire to opt out of the current mailing.
 */
function _mass_contact_create_opt_out_list($categories) {
  $opt_out_users = array();

  // Check for opt-out unless overridden by admin.
  $optout_setting = variable_get('mass_contact_optout_d', 0);

  // Allow users to opt-out of mass e-mails:
  // 0 == 'No', 1 == 'Yes', 2 == 'Selected categories'.
  if ($optout_setting == 1 || $optout_setting == 2) {
    // Select all active users to determine who has opted out.
    $active_users = db_select('users', 'u')
      ->fields('u', array('uid'))
      ->condition('status', 0, '<>')
      ->execute();
    foreach ($active_users as $user) {
      // Load the user so that we may access his/her data.
      $account = user_load($user->uid);
      if ($optout_setting == 1 && $account->mass_contact_optout == 1) {
        // If the user opted out of all messages, add their user id to the
        // list.
        $opt_out_users[$user->uid] = 1;
      }
      elseif ($optout_setting == 2) {
        // Cycle through all the categories selected for this message.
        foreach ($categories as $category) {
          $optout_cid = 'mass_contact_optout_' . $category->cid;
          if ($account->$optout_cid == 1) {
            // If the user opted out of this particular category, add their
            // user id to the list.
            $opt_out_users[$user->uid] = 1;
          }
        }
      }
    }
  }

  return $opt_out_users;
} // End of _mass_contact_create_opt_out_list().


/**
 * Creates a list of recipients.
 *
 * @param roles
 *   An array of roles to get users from.
 * @param opt_out_users
 *   An array of user ids for users who've indicated they want to be excluded
 *   from mass mailings. This list can be obtained by calling the
 *   _mass_contact_create_opt_out_list() function.
 *
 * @return
 *   An array of users' e-mail addresses, keyed by their user name.
 */
function _mass_contact_create_recipient_list($roles, $opt_out_users = array()) {
  global $user;

  $recipient_list = array();

  foreach ($roles as $role) {
    // Get the list of users in this role.
    if ($role == 2) {
      $query = "SELECT u.uid FROM {users} u WHERE u.status <> 0";
      $recipient_ids = db_query($query);
    }
    else {
      $query = "SELECT ur.uid FROM {users_roles} ur LEFT JOIN {users} u ON ur.uid = u.uid WHERE ur.rid = :ur_rid AND u.status <> 0";
      $recipient_ids = db_query($query, array(':ur_rid' => $role));
    }
//    $recipient_ids = db_query($query, array(':ur_rid' => $role))->fetchObject();
//    $recipient_ids = db_select('users_roles', 'ur')
//      ->fields('ur', array('uid'))
//      ->leftJoin('users', 'u', 'ur.uid = u.uid')
//      ->condition('rid', $role)
//      ->condition('status', 0, '<>')
//      ->execute();

    foreach ($recipient_ids as $recipient_id) {
      $recipient = db_select('users', 'u')
        ->fields('u', array('name', 'mail'))
        ->condition('uid', $recipient_id->uid)
        ->execute()
        ->fetchObject();

      if (isset($opt_out_users[$recipient_id->uid])) {
        if ($recipient->name != $user->name && !$opt_out_users[$recipient_id->uid]) {
          $recipient_list[$recipient->name] = $recipient->mail;
        }
      }
      else {
        if ($recipient->name != $user->name) {
          $recipient_list[$recipient->name] = $recipient->mail;
        }
      }
    }
  }

  return $recipient_list;
} // End of _mass_contact_create_recipient_list().


/**
 * Save a copy of the sent message as a node.
 *
 * @param subject
 *   The subject of the message and the title of the node.
 * @param body
 *   The body of the message and node.
 * @param from
 *   The e-mail address representing the user that is sending the message.
 * @param categories
 *   The categories of users sent to.
 * @param roles
 *   The various roles the categories represent.
 * @param recipients
 *   The users who received the message.
 * @param uid
 *   The user who sent the message is also the user who owns the node.
 */
//function _mass_contact_save_node($subject, $body, $recipients, $category, $roles, $uid, $has_attachments) {
function _mass_contact_save_node($params, $body, $from, $category, $recipients, $has_attachments, $result) {
  // Create a new node object.
  $node = new stdClass();
  // Set the node type.
  $node->type = 'mass_contact';
  // Set some default values.
  node_object_prepare($node);
  // Set the user id of the node to that of the current user.
  global $user;
  $node->uid = $user->uid;
  // Or any language code if Locale module is enabled.
  global $language;
//  $node->language = ($language) ? $language->language  : LANGUAGE_NONE;
  $node->language = LANGUAGE_NONE;

  // Set some states.
  $node->status = 0;
  $node->promote = 0;
  $node->sticky = 0;
  $node->comment = 0;

  // Set the title of the node to the subject of the message.
  $node->title = $params['subject'];

  // Set the body of the node to the body of the message.
  $node->body[$node->language][0]['value'] = $body;
  // Set the teaser or summary of the body.
//  $node->body[$node->language][0]['summary'] = text_summary($body);
  // Set the input filter format of the message body.
//  $node->body[$node->language][0]['format'] = 'plain_text';
//  $node->body = '<p><i>' . t('Category:') . ' ' . $category . '</i><p><i>' . t('Roles:') . ' ' . $rolesent . '</i><p><i>' . t('Recipients:') . ' ' . $recipients . '</i><p>' . $body;

  // Set the From field.
  $node->field_mass_contact_from[$node->language][0]['value'] = $from;
  // Set the Category field.
  $node->field_mass_contact_category[$node->language][0]['value'] = $category;
  // Set the To field.
  $to = explode(',', $recipients);
  foreach ($to as $recipient) {
    $node->field_mass_contact_to[$node->language][]['value'] = $recipient;
  }
  // Set the BCC field.
  if (!empty($params['headers']['Bcc'])) {
    $to = explode(',', $params['headers']['Bcc']);
    foreach ($to as $recipient) {
      $node->field_mass_contact_bcc[$node->language][]['value'] = $recipient;
    }

  }
  // Set the Headers field.
  if (!empty($params['headers'])) {
    // Format the output so that it is readable.
    $headers = '';
    foreach ($params['headers'] as $header => $value) {
      $headers .= $header . ": " . $value . "\n";
    }

    $node->field_mass_contact_headers[$node->language][0]['value'] = $headers;
//    $node->field_mass_contact_headers[$node->language][0]['format'] = 'filtered_html';
  }
  // Set the Result field.
  $node->field_mass_contact_result[$node->language][0]['value'] = $result;
//  $node->field_mass_contact_result[$node->language][0]['format'] = 'plain_text';
//dpm($node);

/*
// Some file on our system
$file_path = drupal_realpath('somefile.png'); // Create a File object
$file = (object) array(
          'uid' => 1,
          'uri' => $file_path,
          'filemime' => file_get_mimetype($filepath),
          'status' => 1,
 );
$file = file_copy($file, 'public://'); // Save the file to the root of the files directory. You can specify a subdirectory, for example, 'public://images'
$node->field_image[LANGUAGE_NONE][0] = (array)$file; //associate the file object with the image field:

  // Add any attachments, if they exist.
  if ($has_attachments) {
    for ($i = 1; $i <= variable_get('mass_contact_number_of_attachments', '3'); $i++) {
      if ($_FILES['files']['error']['attachment_' . $i] == UPLOAD_ERR_OK) {
        // Create and populate a file object.
        $file = new stdClass();
        $file->uid = $uid;
        $file->filename = $_FILES['files']['name']['attachment_' . $i];
        $file->filepath = $_FILES['files']['tmp_name']['attachment_' . $i];
        $file->filemime = $_FILES['files']['type']['attachment_' . $i];
        $file->filesize = $_FILES['files']['size']['attachment_' . $i];
        $file->status = ~FILE_STATUS_PERMANENT;
        $file->timestamp = REQUEST_TIME;

        $file->filesource = $_FILES['files']['name']['attachment_' . $i];
        $file->list = 1;
        $file->new = TRUE;

        // Copy the file to Drupal's 'files' directory.
        if (!file_copy($file)) {
          drupal_set_message(t('Failed to save attachment: @filename', array('@filename' => $_FILES['files']['name']['attachment_' . $i])));
          continue;
        }

        // Save the file information to the database.
        drupal_write_record('files', $file);

        // Change file's status to permanent.
        $file->status &= FILE_STATUS_PERMANENT;
        $file = file_save($file);

        // Add the attachment to the node.
        $node->files[$file->fid] = $file;
      }
    }
  }
*/
  // Prepare node for a saving.
  $node = node_submit($node);
  // Save the node.
  node_save($node);

  // Log node creation.
  watchdog('content', 'Mass Contact content added "%title".', array('%title' => $params['subject']), WATCHDOG_NOTICE, l(t('View'), 'node/' . $node->nid));

  // Inform the user.
  drupal_set_message(t('A copy has been created as a node.'));
} // End of _mass_contact_save_node().
