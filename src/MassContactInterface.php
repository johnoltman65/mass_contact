<?php

namespace Drupal\mass_contact;

/**
 * Defines an interface for the Mass Contact helper service.
 */
interface MassContactInterface {

  /**
   * User opt-out is disabled.
   */
  const OPT_OUT_DISABLED = 'disabled';

  /**
   * Global opt-out enabled.
   */
  const OPT_OUT_GLOBAL = 'global';

  /**
   * Per-category opt-out enabled.
   */
  const OPT_OUT_CATEGORY = 'category';

  /**
   * Determines if HTML emails are supported.
   *
   * @return bool
   *   Returns TRUE if the system is capable of sending HTML emails.
   */
  public function htmlSupported();

  /**
   * Main entry point for queuing mass contact emails.
   *
   * @param array $categories
   *   An array of category IDs to send to.
   * @param string $subject
   *   The message subject.
   * @param string $body
   *   The message body.
   * @param string $format
   *   The filter format to use for the body.
   * @param array $configuration
   *   An array of configuration. Default values are provided by the mass
   *   contact settings.
   */
  public function processMassContactMessage(array $categories, $subject, $body, $format, array $configuration = []);

}
