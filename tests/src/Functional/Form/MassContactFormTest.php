<?php

namespace Drupal\Tests\mass_contact\Form;

use Drupal\Core\Test\AssertMailTrait;
use Drupal\Core\Url;
use Drupal\Tests\mass_contact\Functional\MassContactTestBase;
use Drupal\user\Entity\Role;

/**
 * Tests for the Mass Contact form.
 *
 * @group mass_contact
 */
class MassContactFormTest extends MassContactTestBase {

  use AssertMailTrait;

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
   * The role to send to.
   *
   * @var \Drupal\user\RoleInterface
   */
  protected $recipientRole;

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

    // Add 42 users.
    $this->recipientRole = Role::load($this->createRole([]));
    foreach (range(1, 42) as $i) {
      $account = $this->createUser();
      if ($i == 5) {
        // Block the 5th one.
        $account->block();
      }
      $account->addRole($this->recipientRole->id());
      $account->save();
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
    $this->assertSession()->fieldExists('mail');
    $this->assertSession()->fieldValueEquals('mail', $this->massContactUser->getEmail());
    $this->assertSession()->fieldExists('name');
    $this->assertSession()->fieldValueEquals('name', $this->massContactUser->getDisplayName());

    // Update some options.
    $config = $this->config('mass_contact.settings');
    $config->set('bcc_d', FALSE);
    $config->set('create_archive_copy', FALSE);
    $config->set('default_sender_email', 'foo@bar.com');
    $config->set('default_sender_name', 'Foo Bar');
    $config->save();
    $this->massContactRole->grantPermission('mass contact send to users in the ' . $this->categories[3]->id() . ' category')->save();
    $this->drupalGet(Url::fromRoute('mass_contact'));
    $this->assertSession()->statusCodeEquals(200);
    $this->assertSession()->pageTextContains('Recipients will NOT be hidden from each other.');
    $this->assertSession()->pageTextContains(' A copy of this message will NOT be archived on this website.');
    $this->assertSession()->fieldNotExists('mail');
    $this->assertSession()->fieldNotExists('name');

    // Set category 2 to send to all authenticated users.
    $recipients = [
      'role' => [
        'conjunction' => 'AND',
        'categories' => [
          $this->recipientRole->id(),
        ],
      ],
    ];
    $this->categories[2]->setRecipients($recipients);
    $this->categories[2]->save();

    // Send a message to category 2.
    $edit = [
      'subject' => $this->randomString(),
      'message[value]' => $this->randomString(),
      'categories[]' => [$this->categories[2]->id()],
    ];
    $this->drupalPostForm(NULL, $edit, t('Send email'));
    // Should be one item in the queue.
    $queue = \Drupal::queue('mass_contact_queue_messages');
    $this->assertEquals(1, $queue->numberOfItems());

    // Process the queue.
    /** @var \Drupal\Core\Queue\QueueWorkerManagerInterface $manager */
    $manager = $this->container->get('plugin.manager.queue_worker');
    $queue_worker = $manager->createInstance('mass_contact_queue_messages');
    while ($item = $queue->claimItem()) {
      $queue_worker->processItem($item->data);
      $queue->deleteItem($item);
    }

    // There should now be 3 items in the sending queue.
    // @see \Drupal\mass_contact\MassContact::MAX__QUEUE_RECIPIENTS
    $queue = \Drupal::queue('mass_contact_send_message');
    $this->assertEquals(3, $queue->numberOfItems());
    $queue_worker = $manager->createInstance('mass_contact_send_message');
    while ($item = $queue->claimItem()) {
      $queue_worker->processItem($item->data);
      $queue->deleteItem($item);
    }

    // Should be 41 emails (41 non-blocked users with the recipient role).
    $emails = $this->getMails();
    $this->assertEquals(41, count($emails));
  }

}
