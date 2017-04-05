<?php

namespace Drupal\mass_contact;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;

/**
 * The Mass Contact helper service.
 */
class MassContact implements MassContactInterface {

  /**
   * Defines the HTML modules supported.
   *
   * @var string[]
   */
  protected static $htmlEmailModules = [
    'mimemail',
    'swiftmailer',
  ];

  /**
   * The mass contact settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   */
  protected $config;

  /**
   * The module handler.
   *
   * @var \Drupal\Core\Extension\ModuleHandlerInterface
   */
  protected $moduleHandler;

  /**
   * Constructs the Mass Contact helper.
   *
   * @param \Drupal\Core\Extension\ModuleHandlerInterface $module_handler
   *   The module handler service.
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   */
  public function __construct(ModuleHandlerInterface $module_handler, ConfigFactoryInterface $config_factory) {
    $this->moduleHandler = $module_handler;
    $this->config = $config_factory->get('mass_contact.settings');
  }

  /**
   * {@inheritdoc}
   */
  public function htmlSupported() {
    foreach (static::$htmlEmailModules as $module) {
      if ($this->moduleHandler->moduleExists($module)) {
        return TRUE;
      }
    }
    return FALSE;
  }

}
