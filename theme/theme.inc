<?php

/**
 * @file
 * Any hooks/functions used by this module'ss themes.
 */

/**
 * Convert a mm measurement (as found in a rend attribute) to a px measurement.
 * 
 * @param string $rend The contents of the render attribute
 * @param boolean $isFont If true, then the minimum font size is applied.
 * 
 * @return string The converted measurement with 'px' appended.
 */
function mmToPx($rend, $isFont) {
  $matches = array();
  if (!preg_match('/([0-9.]+)mm/i', $rend, $matches)) {
    error_log('unparseable measurement ' . $rend);
  }
  if ($isFont) {
    return max(array(BALLADS_VIEWER_MIN_FONT_PX, BALLADS_VIEWER_PX_PER_MM * $matches[1])) . 'px';
  }
  else {
    return BALLADS_VIEWER_PX_PER_MM * $matches[1] . 'px';
  }
}

/**
 * Convert a rend attribute to a CSS style.
 * 
 * @param string $rend Contents of the rend attribute.
 * @return string|null The converted CSS style or null if the attribute is unparseable.
 */
function rendToStyle($rend) {
  $matches = array();
  if (!preg_match('/^([a-z][a-z0-9-]*)/i', $rend, $matches)) {
    error_log('unparseable rendering ' . $rend);
    return NULL;
  }
  switch (strtolower($matches[1])) {
    case 'center':
    case 'centre':
      return 'text-align: center';

    case 'italic':
    case 'italics':
      return 'font-style: italic';

    case 'roman':
      return 'font-style: normal';

    case 'uppercase':
      return 'text-transform: uppercase';

    case 'font-size':
      return 'font-size: ' . mmToPx($rend, TRUE);

    case 'indent':
      return 'margin-left: ' . mmToPx($rend, FALSE);

    case 'length':
      return 'width: ' . mmToPx($rend, FALSE);

    default:
        error_log('unknown rendering: ' . $rend);
      return NULL;
  }
}

/**
 * Implements hook_preprocess_theme().
 * 
 * Post-processes the DOM to fix the problems in the HTML
 * caused by the limitations of XSLT 1.0. Fixes the following things:
 * 
 * The src attribute on image element.
 * 
 * Converts TEI rend attributes (stored in data-rend) to CSS styles.
 * 
 * Fixes links in the table of contents by finding the corresponding Islandora
 * object based on its MODS identifier.
 */
function ballads_viewer_preprocess_ballads_viewer(array &$variables) {
  $path = drupal_get_path('module', 'ballads_viewer');
  drupal_add_css($path . '/css/ballads.css');

  if (!isset($variables['islandora_object'])) {
    return;
  }

  $islandora_object = $variables['islandora_object'];
  $variables['parent_collections'] = islandora_get_parents_from_rels_ext($islandora_object);
  $obj_xml_content = $islandora_object['OBJ']->content;

  $object_render_stylesheet = null;
  if ($islandora_object['RENDER_STYLESHEET']) {
    $object_render_stylesheet = $islandora_object['RENDER_STYLESHEET']->content;
  } elseif ($variables['parent_collections'][0]['RENDER_STYLESHEET']) {
    $object_render_stylesheet = $variables['parent_collections'][0]['RENDER_STYLESHEET']->content;
  }

  if ($object_render_stylesheet === null) {
    dsm("No configured stylesheet.");
    $variables['islandora_content'] = '<pre>' . htmlspecialchars($obj_xml_content) . '</pre>';
    return;
  }
  module_load_include('inc', 'islandora_solution_pack_xml', 'includes/utilities');
  $xml_dom = ballads_viewer_apply_xslt($object_render_stylesheet, $obj_xml_content, true);
  
  // fix image links.
  foreach ($xml_dom->getElementsByTagName('img') as $img) {
    $img->setAttribute('src', '/islandora/object/' . $islandora_object->id . '/' . $img->getAttribute('src'));
  }
  
  // fix nodes with TEI rend attributes.
  $xp = new DOMXpath($xml_dom);
  foreach ($xp->query('//*[@data-rend]') as $node) {
    $rend = $node->getAttribute('data-rend');
    $styles = array_map('rendToStyle', preg_split('/\s+/', $rend));
    $node->setAttribute('style', implode('; ', $styles));
    $node->removeAttribute('data-rend');
  }  
  
    // fix the Table of Contents.
  foreach ($xp->query('//*[@class="toc-link"]') as $node) {
    $filename = $node->getAttribute('href');
    $solrBase = variable_get('islandora_solr_url', 'http://localhost:8080/solr');
    $result = file_get_contents("{$solrBase}/select?version=1.2&wt=json&json.nl=map&q=mods_identifier_original_filename_ms:%22{$filename}%22&rows=1&fl=PID&omitHeader=true");
    $json = json_decode($result, TRUE);
    $PID = $json['response']['docs'][0]['PID'];
    $node->setAttribute('href', "/islandora/object/{$PID}");
  }
  
  $variables['islandora_content'] = $xml_dom->saveXML();
}
