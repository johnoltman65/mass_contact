<?php

namespace Drupal\mass_contact\Entity;

use Drupal\Core\Config\Entity\ConfigEntityInterface;

/**
 * Provides an interface for defining Mass contact categories.
 */
interface MassContactCategoryInterface extends ConfigEntityInterface {

  /**
   * Gets the recipient category plugin definitions.
   *
   * @return array
   *   An array of configured selection plugins, keyed by plugin ID.
   */
  public function getGroupings();

  /**
   * Sets grouping definitions.
   *
   * @param array $groupings
   *   The grouping configurations, keyed by plugin ID.
   */
  public function setGroupings(array $groupings);

  /**
   * Determines if this category should be selected by default on mass contacts.
   *
   * @return bool
   *   Returns TRUE if the category should be selected by default.
   */
  public function getSelected();

  /**
   * Sets category to be selected by default.
   *
   * @param bool $selected
   *   Set to TRUE if the category should be selected by default.
   */
  public function setSelected($selected);

}