<?php

namespace Drupal\mass_contact\Entity;

use Drupal\Core\Config\Entity\ConfigEntityBase;
use Drupal\mass_contact\Plugin\MassContact\GroupingMethod\GroupingInterface;

/**
 * Defines the Mass contact category entity.
 *
 * @ConfigEntityType(
 *   id = "mass_contact_category",
 *   label = @Translation("Mass contact category"),
 *   handlers = {
 *     "list_builder" = "Drupal\mass_contact\CategoryListBuilder",
 *     "form" = {
 *       "add" = "Drupal\mass_contact\Form\CategoryForm",
 *       "edit" = "Drupal\mass_contact\Form\CategoryForm",
 *       "delete" = "Drupal\mass_contact\Form\CategoryDeleteForm"
 *     },
 *     "route_provider" = {
 *       "html" = "Drupal\Core\Entity\Routing\AdminHtmlRouteProvider"
 *     }
 *   },
 *   config_prefix = "category",
 *   admin_permission = "mass contact administer",
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "label",
 *     "uuid" = "uuid"
 *   },
 *   config_export = {
 *     "id",
 *     "label",
 *     "recipients",
 *     "selected"
 *   },
 *   links = {
 *     "canonical" = "/admin/config/mass-contact/category/{mass_contact_category}/edit",
 *     "add-form" = "/admin/config/mass-contact/category/add",
 *     "edit-form" = "/admin/config/mass-contact/category/{mass_contact_category}/edit",
 *     "delete-form" = "/admin/config/mass-contact/category/{mass_contact_category}/delete",
 *     "collection" = "/admin/config/mass-contact/category"
 *   }
 * )
 */
class MassContactCategory extends ConfigEntityBase implements MassContactCategoryInterface {

  /**
   * The Mass contact category ID.
   *
   * @var string
   */
  protected $id;

  /**
   * The Mass contact category label.
   *
   * @var string
   */
  protected $label;

  /**
   * The recipient categories, keyed by plugin ID.
   *
   * The structure of each item is, for instance:
   * @code
   *   categories:
   *     - role_1
   *     - role_2
   * @endcode
   *
   * @var array
   */
  protected $recipients;

  /**
   * Boolean indicating if this category should be selected by default.
   *
   * @var bool
   */
  protected $selected;

  /**
   * {@inheritdoc}
   */
  public function getGroupings() {
    return $this->recipients;
  }

  /**
   * {@inheritdoc}
   */
  public function setGroupings(array $groupings) {
    $this->recipients = $groupings;
  }

  /**
   * {@inheritdoc}
   */
  public function getSelected() {
    return $this->selected;
  }

  /**
   * {@inheritdoc}
   */
  public function setSelected($selected) {
    $this->selected = $selected;
  }

  /**
   * {@inheritdoc}
   */
  public function getGroupingCategories($grouping_id) {
    $groupings = $this->getGroupings();
    if (isset($groupings[$grouping_id])) {
      return $groupings[$grouping_id];
    }
    return [];
  }

}
