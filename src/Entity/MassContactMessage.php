<?php

namespace Drupal\mass_contact\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;

/**
 * The mass contact message entity for archiving messages.
 *
 * @ContentEntityType(
 *   id = "mass_contact_message",
 *   label = @Translation("Mass Contact Message"),
 *   label_singular = @Translation("mass contact message"),
 *   label_plural = @Translation("mass contact messages"),
 *   label_count = @PluralTranslation(
 *     singular = "@count mass contact message",
 *     plural = "@count mass contact messages"
 *   ),
 *   translatable = FALSE,
 *   base_table = "mass_contact",
 *   data_table = "mass_contact_field_data",
 *   entity_keys = {
 *     "id" = "id",
 *     "uuid" = "uuid",
 *     "uid" = "uid",
 *     "label" = "subject"
 *   }
 * )
 */
class MassContactMessage extends ContentEntityBase {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type) {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['subject'] = BaseFieldDefinition::create('string')
      ->setLabel(t('Subject'))
      ->setRequired(TRUE)
      ->setTranslatable(FALSE)
      ->setRevisionable(FALSE)
      ->setSetting('max_length', 255)
      ->setDisplayOptions('view', [
        'label' => 'hidden',
        'type' => 'string',
        'weight' => -5,
      ]);

    return $fields;
  }

}
