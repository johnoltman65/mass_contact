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

  /**
   * Takes a mass contact, calculates recipients and queues them for delivery.
   *
   * @param array $category_ids
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
  public function queueRecipients(array $category_ids, $subject, $body, $format, array $configuration = []);

  /**
   * Sends a message to a list of recipient user IDs.
   *
   * @param int[] $recipients
   *   An array of recipient user IDs.
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
  public function sendMessage(array $recipients, $subject, $body, $format, array $configuration = []);

  /**
   * Given categories, returns an array of recipient IDs.
   *
   * @param string[] $category_ids
   *   An array of mass contact category IDs.
   *
   * @return int[]
   *   An array of recipient user IDs.
   */
  public function getRecipients(array $category_ids);

  /**
   * Get groups of recipients for batch processing.
   *
   * @param int[] $all_recipients
   *   An array of all recipients.
   *
   * @return array
   *   An array of arrays of recipients.
   */
  public function getGroupedRecipients(array $all_recipients);

}
