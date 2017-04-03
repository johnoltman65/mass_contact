<?php

namespace Drupal\mass_contact\Plugin\MassContact\GroupingMethod;
use Drupal\Component\Plugin\ConfigurablePluginInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Plugin\PluginFormInterface;

/**
 * Defineds a grouping method interface.
 */
interface Grouping  {

  /**
   * Retrieve the list of users by category.
   *
   * @param array $categories
   *   An array of category IDs for which to retrieve users. For instance,
   *   in the role grouping this would be an array of role IDs.
   *
   * @return int[]
   *   An array of recipient user IDs.
   */
  public function getRecipients(array $categories);

  /**
   * Display list of categories.
   *
   * @param array $categories
   *   An array of category IDs.
   *
   * @return string
   *   Display included categories as a string.
   */
  public function displayCategories(array $categories);

  /**
   * Builds the form for selecting categories for a mass contact.
   *
   * @param array $form
   *   The form definition array.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   The form state object.
   *
   * @return array
   *   The form element.
   */
  public function adminForm(array $form, FormStateInterface $form_state);

  /* @todo The rest
  // The next three callbacks are used to maintain the form for adding/editing
  // categories.
'mass_contact_admin_edit' => 'mass_contact_taxonomy_admin_edit',
'mass_contact_admin_edit_validate' => 'mass_contact_taxonomy_admin_edit_validate',
'mass_contact_admin_edit_submit' => 'mass_contact_taxonomy_admin_edit_submit',

  // This callback is used to maintain the form for opting in or out of
  // categories.
'mass_contact_user_edit' => 'mass_contact_taxonomy_user_edit',
);
*/
}
