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
  $rows = [];

  $results = db_select('mass_contact', 'mc')
    ->fields('mc', ['cid', 'category', 'recipients', 'selected'])
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
    $recipients = [];
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

  $header = [
    t('Category'),
    t('Recipients'),
    t('Selected'),
    [
      'data' => t('Operations'),
      'colspan' => 2,
    ],
  ];

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
 * @param int $cid
 *   The id of the category to edit. If NULL, then add rather than edit.
 *
 * @return array
 *   An associative array that defines the form to be built.
 */
function mass_contact_admin_edit($form, $form_state, $cid = NULL) {
  // Initialize the array.
  $edit = [
    'category' => '',
    'recipients' => '',
    'selected' => '',
    'cid' => '',
  ];

  if (arg(4) == "edit" && $cid > 0) {
    // Get the information about the category being edited.
    $edit = db_select('mass_contact', 'mc')
      ->fields('mc')
      ->condition('cid', $cid)
      ->execute()
      ->fetchAssoc();
  }

  $form['category'] = [
    '#type' => 'textfield',
    '#title' => t('Category'),
    '#maxlength' => 255,
    '#default_value' => $edit['category'],
    '#description' => t("Will appear in the subject of your email as [category]."),
    '#required' => TRUE,
  ];

  $form['recipients'] = ['#tree' => TRUE];

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

  $form['selected_categories'] = [
    '#type' => 'fieldset',
    '#title' => t('Selected categories'),
  ];
  $form['selected_categories']['selected'] = [
    '#type' => 'select',
    '#title' => t('Selected'),
    '#options' => [
      '0' => t('No'),
      '1' => t('Yes'),
    ],
    '#default_value' => $edit['selected'],
    '#description' => t('Set this to <em>Yes</em> if you would like this category to be selected by default.'),
  ];
  $form['selected_categories']['reset_selected'] = [
    '#type' => 'checkbox',
    '#title' => t('Reset all previously selected categories to <em>No</em>'),
  ];

  $form['cid'] = [
    '#type' => 'value',
    '#value' => $edit['cid'],
  ];

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Save'),
  ];

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
      ->fields(['selected' => 0])
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
  $recipients = [];
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

  $record = [
    'category' => $form_state['values']['category'],
    'recipients' => $recipients,
    'reply' => $form_state['values']['reply'],
    'weight' => $form_state['values']['weight'],
    'selected' => $form_state['values']['selected'],
  ];
  if (arg(4) == 'add') {
    \Drupal::database()->insert('mass_contact')->fields($record)->execute();
    drupal_set_message(t('Category %category has been added.', ['%category' => $form_state['values']['category']]));
    // @FIXME
    // l() expects a Url object, created from a route name or external URI.
    // watchdog('mass_contact', 'Mass Contact form: category %category added.', array('%category' => $form_state['values']['category']), WATCHDOG_NOTICE, l(t('view'), 'admin/config/system/mass_contact'));
  }
  else {
    $record['cid'] = $form_state['values']['cid'];
    \Drupal::database()->merge('mass_contact')->fields($record)->key(['cid'])->execute();
    drupal_set_message(t('Category %category has been updated.', ['%category' => $form_state['values']['category']]));
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
 * @param int $cid
 *   The id of the category to delete.
 *
 * @return array
 *   A confirmation form for the user to acknowledge.
 */
function mass_contact_admin_delete($form, $form_state, $cid = NULL) {
  $info = db_select('mass_contact', 'mc')
    ->fields('mc', ['category'])
    ->condition('cid', $cid)
    ->execute()
    ->fetchObject();
  if ($info) {
    $form['category'] = [
      '#type' => 'value',
      '#value' => $info->category,
    ];

    return confirm_form($form, t('Are you sure you want to delete %category?', ['%category' => $info->category]), 'admin/config/system/mass_contact', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
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
  drupal_set_message(t('Category %category has been deleted.', ['%category' => $form_state['values']['category']]));
  \Drupal::logger('mass_contact')->notice('Mass Contact form: category %category deleted.', ['%category' => $form_state['values']['category']]);
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

  return system_settings_form($form);
}
