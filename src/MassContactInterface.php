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

}
