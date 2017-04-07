<?php

namespace Drupal\mass_contact\Plugin\MassContact\GroupingMethod;

use Drupal\Component\Plugin\PluginBase;
use Drupal\Core\StringTranslation\StringTranslationTrait;

/**
 * Base class for grouping method plugins.
 */
abstract class GroupingBase extends PluginBase implements GroupingInterface {

  use StringTranslationTrait;

  /**
   * {@inheritdoc}
   */
  public function getConfiguration() {
    return $this->configuration;
  }

  /**
   * {@inheritdoc}
   */
  public function setConfiguration(array $configuration) {
    $this->configuration = NestedArray::mergeDeep($this->defaultConfiguration(), $configuration);
  }

  /**
   * {@inheritdoc}
   */
  public function defaultConfiguration() {
    return [
      'categories' => [],
      'conjunction' => 'ANY',
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function getCategories() {
    return $this->configuration['categories'];
  }

}
