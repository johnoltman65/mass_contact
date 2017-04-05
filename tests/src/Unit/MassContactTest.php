<?php

namespace Drupal\Tests\mass_contact\Unit;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\mass_contact\MassContact;
use Drupal\Tests\UnitTestCase;

/**
 * Unit tests for the Mass Contact helper service.
 *
 * @group mass_contact
 *
 * @coversDefaultClass \Drupal\mass_contact\MassContact
 */
class MassContactTest extends UnitTestCase {

  /**
   * Tests html support detection.
   *
   * @covers ::htmlSupported
   */
  public function testHtmlSupported() {
    // Test for no modules supporting html email.
    $module_handler = $this->prophesize(ModuleHandlerInterface::class);
    $module_handler->moduleExists('mimemail')->willReturn(FALSE);
    $module_handler->moduleExists('swiftmailer')->willReturn(FALSE);
    $config_factory = $this->prophesize(ConfigFactoryInterface::class)->reveal();
    $fixture = new MassContact($module_handler->reveal(), $config_factory);
    $this->assertFalse($fixture->htmlSupported());

    // Mime mail module.
    $module_handler->moduleExists('mimemail')->willReturn(TRUE);
    $fixture = new MassContact($module_handler->reveal(), $config_factory);
    $this->assertTrue($fixture->htmlSupported());

    // Swiftmailer module.
    $module_handler->moduleExists('mimemail')->willReturn(FALSE);
    $module_handler->moduleExists('swiftmailer')->willReturn(TRUE);
    $fixture = new MassContact($module_handler->reveal(), $config_factory);
    $this->assertTrue($fixture->htmlSupported());
  }

}
