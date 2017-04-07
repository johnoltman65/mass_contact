<?php

namespace Drupal\Tests\mass_contact\Form;

use Drupal\Core\Url;
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
   * Some test categories.
   *
   * @var \Drupal\mass_contact\Entity\MassContactCategoryInterface[]
   */
  protected $categories;

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();

    $this->massContactUser = $this->createUser(['mass contact send messages']);
    $role_id = $this->massContactUser->getRoles(TRUE);
    $this->massContactRole = Role::load(reset($role_id));

    foreach (range(1, 6) as $i) {
      $this->categories[$i] = $this->createCategory();
    }
  }

  /**
   * Tests basic form operation on an unprivileged user.
   */
  public function testNormalAccess() {
    $this->drupalLogin($this->massContactUser);
    // With access to no categories, an error should appear.
    $this->drupalGet(Url::fromRoute('mass_contact'));
    $this->assertSession()->statusCodeEquals(200);
    $this->assertSession()->pageTextContains('No categories found!');

    // Grant permission to one category only.
    $this->massContactRole->grantPermission('mass contact send to users in the ' . $this->categories[2]->id() . ' category')->save();
    $this->drupalGet(Url::fromRoute('mass_contact'));
    $this->assertSession()->statusCodeEquals(200);
    $this->assertSession()->pageTextContains('This message will be sent to all users in the ' . $this->categories[2]->label() . ' category.');
    $this->assertSession()->pageTextContains('A copy of this message will be archived on this website.');
    $this->assertSession()->pageTextContains('Recipients will be hidden from each other.');

    // Update some options.
    $config = $this->config('mass_contact.settings');
    $config->set('bcc_d', FALSE);
    $config->set('create_archive_copy', FALSE);
    $config->save();
    $this->massContactRole->grantPermission('mass contact send to users in the ' . $this->categories[2]->id() . ' category')->save();
    $this->drupalGet(Url::fromRoute('mass_contact'));
    $this->assertSession()->statusCodeEquals(200);
    $this->assertSession()->pageTextContains('Recipients will NOT be hidden from each other.');
    $this->assertSession()->pageTextContains(' A copy of this message will NOT be archived on this website.');

    // Send a message.
    $edit = [
      'subject' => $this->randomString(),
      'message[value]' => $this->randomString(),
    ];
    $this->drupalPostForm(NULL, $edit, t('Send email'));
    // Should be one item in the queue.
    $queue = \Drupal::queue('mass_contact_queue_messages');
    $this->assertEquals(1, $queue->numberOfItems());

    // @todo process queue and verify emails.
  }

}
