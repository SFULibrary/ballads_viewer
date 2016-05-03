<?php

/**
 * @file
 * Handles the display/submission of the admin settings form for this module.
 */

/**
 * Defines the admin settings form.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The Drupal form definition.
 */
function ballads_viewer_admin(array $form, array &$form_state) {
  
  $form['ballads_viewer_linkxml'] = array(
    '#type' => 'checkbox',
    '#title' => t('Link to source'),
    '#description' => t('Link to the source XML.'),
    '#default_value' => variable_get('ballads_viewer_linkxml', TRUE),
  );
  return system_settings_form($form);
}
