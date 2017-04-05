<?php

namespace Drupal\mass_contact\Form;

use Drupal\Core\Entity\EntityForm;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Plugin\DefaultPluginManager;
use Drupal\Core\Url;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Class CategoryForm.
 *
 * @package Drupal\mass_contact\Form
 */
class CategoryForm extends EntityForm {

  /**
   * GroupingInterface method plugin manager.
   *
   * @var \Drupal\Core\Plugin\DefaultPluginManager
   */
  protected $groupingMethodManager;

  /**
   * Constructs the mass contact category form.
   *
   * @param \Drupal\Core\Plugin\DefaultPluginManager $grouping_method_manager
   *   The grouping method plugin manager form.
   */
  public function __construct(DefaultPluginManager $grouping_method_manager) {
    $this->groupingMethodManager = $grouping_method_manager;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static($container->get('plugin.manager.mass_contact.grouping_method'));
  }

  /**
   * {@inheritdoc}
   */
  public function form(array $form, FormStateInterface $form_state) {
    $form = parent::form($form, $form_state);

    /** @var \Drupal\mass_contact\Entity\MassContactCategoryInterface $mass_contact_category */
    $mass_contact_category = $this->entity;
    $form['label'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Category'),
      '#maxlength' => 255,
      '#default_value' => $mass_contact_category->label(),
      '#description' => $this->t('The category name to display on the Mass Contact form.'),
      '#required' => TRUE,
    ];

    $form['id'] = [
      '#type' => 'machine_name',
      '#default_value' => $mass_contact_category->id(),
      '#machine_name' => [
        'exists' => '\Drupal\mass_contact\Entity\MassContactCategory::load',
      ],
      '#disabled' => !$mass_contact_category->isNew(),
    ];

    $form['recipients'] = [
      '#type' => 'details',
      '#title' => $this->t('Recipients'),
      '#tree' => TRUE,
      '#open' => TRUE,
    ];

    // Attach plugin forms.
    foreach ($this->groupingMethodManager->getDefinitions() as $definition) {
      /** @var \Drupal\mass_contact\Plugin\MassContact\GroupingMethod\GroupingInterface $plugin */
      $configuration = [
        'categories' => $mass_contact_category->getGroupingCategories($definition['id']),
      ];
      $plugin = $this->groupingMethodManager->createInstance($definition['id'], $configuration);
      $form['recipients'][$plugin->getPluginId()] = $plugin->adminForm($form, $form_state);
    }

    $form['selected'] = [
      '#type' => 'checkbox',
      '#title' => $this->t('Selected by default'),
      '#default_value' => $mass_contact_category->getSelected(),
      '#description' => t('This category will be selected by default on the <a href="@url">Mass Contact form</a>.', ['@url' => Url::fromRoute('mass_contact')->toString()]),
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function save(array $form, FormStateInterface $form_state) {
    /** @var \Drupal\mass_contact\Entity\MassContactCategoryInterface $mass_contact_category */
    $mass_contact_category = $this->entity;
    $groupings = $mass_contact_category->getGroupings();
    foreach ($groupings as $plugin_id => $grouping) {
      $groupings[$plugin_id] = array_values(array_filter($grouping));
    }
    $mass_contact_category->setGroupings($groupings);
    $status = $mass_contact_category->save();

    switch ($status) {
      case SAVED_NEW:
        drupal_set_message($this->t('Created the %label category.', [
          '%label' => $mass_contact_category->label(),
        ]));
        break;

      default:
        drupal_set_message($this->t('Saved the %label category.', [
          '%label' => $mass_contact_category->label(),
        ]));
    }
    $form_state->setRedirectUrl($mass_contact_category->urlInfo('collection'));
  }

}
