<?php

/**
 * @file
 * Any hooks/functions used by this module'ss themes.
 */

/**
 * Implements hook_preprocess_theme().
 */
function ballads_viewer_preprocess_ballads_viewer(array &$variables) {
  if (isset($variables['islandora_object'])) {
    $islandora_object = $variables['islandora_object'];
    $variables['islandora_content'] = htmlspecialchars($islandora_object['OBJ']->content);
  }

  $path = drupal_get_path('module', 'ballads_viewer');
  drupal_add_css($path . '/css/ballads.css');
}
