<?php

namespace Drupal\Tests\mass_contact\Form;

use Drupal\Tests\mass_contact\Functional\MassContactTestBase;
use Drupal\user\Entity\Role;

/**
 * Tests for the Mass Contact form.
 *
 * @group mass_contact
 */
class MassContactFormTest extends MassContactTestBase {

  /**
   * Non admin user.
   *
   * @var \Drupal\user\UserInterface
   */
  protected $massContactUser;

  /**
   * The role for changing mass contact permissions.
   *
   * @var \Drupal\user\RoleInterface
   */
  protected $massContactRole;

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();

    $this->massContactUser = $this->createUser(['mass contact send messages']);
    $role_id = $this->massContactUser->getRoles(TRUE);
    $this->massContactRole = Role::load($role_id);
  }

}
