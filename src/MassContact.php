<?php

namespace Drupal\mass_contact;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\Queue\QueueFactory;

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
   * The message queueing queue.
   *
   * @var \Drupal\Core\Queue\QueueInterface
   */
  protected $processingQueue;

  /**
   * The message sending queue.
   *
   * @var \Drupal\Core\Queue\QueueInterface
   */
  protected $sendingQueue;

  /**
   * Constructs the Mass Contact helper.
   *
   * @param \Drupal\Core\Extension\ModuleHandlerInterface $module_handler
   *   The module handler service.
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Queue\QueueFactory $queue
   *   The queue factory.
   */
  public function __construct(ModuleHandlerInterface $module_handler, ConfigFactoryInterface $config_factory, QueueFactory $queue) {
    $this->moduleHandler = $module_handler;
    $this->config = $config_factory->get('mass_contact.settings');
    $this->processingQueue = $queue->get('mass_contact_queue_messages', TRUE);
    $this->sendingQueue = $queue->get('mass_contact_send_message', TRUE);
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

  /**
   * {@inheritdoc}
   */
  public function processMassContactMessage(array $categories, $subject, $body, $format, array $configuration = []) {
    $configuration += $this->getDefaultConfiguration();
    $data = [
      'categories' => $categories,
      'subject' => $subject,
      'body' => $body,
      'format' => $format,
      'configuration' => $configuration,
    ];
    $this->processingQueue->createItem($data);
  }

  /**
   * Set default config values.
   *
   * @return array
   *   The default configuration as defined in the mass_contact.settings config.
   */
  protected function getDefaultConfiguration() {
    $default = [];
    // @todo
    return $default;
  }

}
