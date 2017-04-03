<?php

namespace Drupal\mass_contact\Form;

use Drupal\Core\Entity\EntityForm;
use Drupal\Core\Form\FormStateInterface;

/**
 * Class CategoryForm.
 *
 * @package Drupal\mass_contact\Form
 */
class CategoryForm extends EntityForm {
  /**
   * {@inheritdoc}
   */
  public function form(array $form, FormStateInterface $form_state) {
    $form = parent::form($form, $form_state);

    $mass_contact_category = $this->entity;
    $form['label'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Label'),
      '#maxlength' => 255,
      '#default_value' => $mass_contact_category->label(),
      '#description' => $this->t("Label for the Mass contact category."),
      '#required' => TRUE,
    );

    $form['id'] = array(
      '#type' => 'machine_name',
      '#default_value' => $mass_contact_category->id(),
      '#machine_name' => array(
        'exists' => '\Drupal\mass_contact\Entity\MassContactCategory::load',
      ),
      '#disabled' => !$mass_contact_category->isNew(),
    );

    /* You will need additional form elements for your custom properties. */

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function save(array $form, FormStateInterface $form_state) {
    $mass_contact_category = $this->entity;
    $status = $mass_contact_category->save();

    switch ($status) {
      case SAVED_NEW:
        drupal_set_message($this->t('Created the %label Mass contact category.', [
          '%label' => $mass_contact_category->label(),
        ]));
        break;

      default:
        drupal_set_message($this->t('Saved the %label Mass contact category.', [
          '%label' => $mass_contact_category->label(),
        ]));
    }
    $form_state->setRedirectUrl($mass_contact_category->urlInfo('collection'));
  }

}
