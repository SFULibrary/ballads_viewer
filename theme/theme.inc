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
    $variables['parent_collections'] = islandora_get_parents_from_rels_ext($islandora_object);
    $obj_xml_content = $islandora_object['OBJ']->content;

    if ($islandora_object['RENDER_STYLESHEET']) {
      module_load_include('inc', 'islandora_solution_pack_xml', 'includes/utilities');
      $object_render_stylesheet = $islandora_object['RENDER_STYLESHEET']->content;
      $variables['islandora_content'] = islandora_solution_pack_xml_apply_xslt($object_render_stylesheet, $obj_xml_content);
    }
    elseif ($variables['parent_collections'][0]['RENDER_STYLESHEET']) {
      module_load_include('inc', 'islandora_solution_pack_xml', 'includes/utilities');
      $object_render_stylesheet = $variables['parent_collections'][0]['RENDER_STYLESHEET']->content;
      $variables['islandora_content'] = islandora_solution_pack_xml_apply_xslt($object_render_stylesheet, $obj_xml_content);
    } else {
      dsm("No configured stylesheet.");
      $variables['islandora_content'] = '<pre>' . htmlspecialchars($obj_xml_content) . '</pre>';
    }
  }

  $path = drupal_get_path('module', 'ballads_viewer');
  drupal_add_css($path . '/css/ballads.css');
}
