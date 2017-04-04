<?php

namespace Drupal\mass_contact\Plugin\MassContact\GroupingMethod;

use Drupal\Component\Utility\NestedArray;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\Core\Plugin\PluginBase;
use Drupal\mass_contact\Entity\MassContactCategoryInterface;
use Drupal\user\RoleInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Select users by their role.
 *
 * @GroupingMethod(
 *   id = "role",
 *   title = @Translation("Role"),
 *   description = @Translation("Select recipients by role")
 * )
 */
class Role extends PluginBase implements GroupingInterface, ContainerFactoryPluginInterface {

  /**
   * The entity type manager service.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;


  /**
   * Constructs the role grouping plugin.
   *
   * @param array $configuration
   *   A configuration array containing information about the plugin instance.
   * @param string $plugin_id
   *   The plugin_id for the plugin instance.
   * @param mixed $plugin_definition
   *   The plugin implementation definition.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager service.
   */
  public function __construct(array $configuration, $plugin_id, $plugin_definition, EntityTypeManagerInterface $entity_type_manager) {
    parent::__construct($configuration, $plugin_id, $plugin_definition);
    $this->configuration += $this->defaultConfiguration();
    $this->entityTypeManager = $entity_type_manager;
  }

  /**
   * @inheritDoc
   */
  public function getConfiguration() {
    return $this->configuration;
  }

  /**
   * @inheritDoc
   */
  public function setConfiguration(array $configuration) {
    $this->configuration = NestedArray::mergeDeep($this->defaultConfiguration(), $configuration);
  }

  /**
   * @inheritDoc
   */
  public function defaultConfiguration() {
    return [
      'categories' => [],
    ];
  }

  /**
   * @inheritDoc
   */
  public function calculateDependencies() {
    $configurations = [];
    foreach ($this->configuration['categories'] as $role_id) {
      $configurations[] = 'user.role.' . $role_id;
    }
    return [
      'config' => $configurations,
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $configuration,
      $plugin_id,
      $plugin_definition,
      $container->get('entity_type.manager')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function displayCategories(array $categories) {
    /** @var \Drupal\user\RoleInterface[] $roles */
    $roles = $this->entityTypeManager->getStorage('user_role')->loadMultiple($categories);
    $labels = [];
    foreach ($roles as $role) {
      $labels[] = $role->label();
    }
    return $this->t('Roles: %roles', ['%roles' => implode(',', $labels)]);
  }

  /**
   * {@inheritdoc}
   */
  public function getRecipients(array $categories) {
    $query = $this->entityTypeManager->getStorage('user')->getQuery();
    $query->condition('status', 1);

    // If the authenticated role is not included, add the appropriate filters.
    // Otherwise, return all active users.
    if (!in_array(RoleInterface::AUTHENTICATED_ID, $categories)) {
      $query->condition('roles', $categories, 'IN');
    }

    return $query->execute();
  }

  /**
   * {@inheritdoc}
   */
  public function adminForm(array $form, FormStateInterface $form_state) {
    $roles = $this->entityTypeManager->getStorage('user_role')->loadMultiple();
    unset($roles[RoleInterface::ANONYMOUS_ID]);
    $options = array_map(function (RoleInterface $role) {
      return $role->label();
    }, $roles);

    // Create a set of checkboxes, including each role.
    $form_element = array(
      '#type' => 'checkboxes',
      '#title' => t('User roles to include'),
      '#options' => $options,
      '#default_value' => $this->configuration['categories'],
      '#description' => t('These roles will be added to the mailing list. Note: if you check "authenticated user", other roles will not be added, as they will receive the email anyway.'),
    );

    return $form_element;

  }

  /**
   * {@inheritdoc}
   */
  public function validateConfigurationForm(array &$form, FormStateInterface $form_state) {
    // TODO: Implement validateConfigurationForm() method.
  }

  /**
   * {@inheritdoc}
   */
  public function submitConfigurationForm(array &$form, FormStateInterface $form_state) {
    // TODO: Implement submitConfigurationForm() method.
  }

}
