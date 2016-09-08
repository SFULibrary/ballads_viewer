<?php

/**
 * @file
 * Any hooks/functions used by this module'ss themes.
 */

/**
 * Convert a mm measurement (as found in a rend attribute) to a px measurement.
 *
 * @param string $rend
 *   The contents of the render attribute.
 * @param bool $is_font
 *   If true, then the minimum font size will be applied.
 *
 * @return string
 *   The converted measurement with 'px' appended.
 */
function ballads_viewer_convert_mm($rend, $is_font) {
  $matches = array();
  if (!preg_match('/([0-9.]+)mm/i', $rend, $matches)) {
    error_log('unparseable measurement ' . $rend);
  }
  if ($is_font) {
    return max(array(
        variable_get('ballads_viewer_min_font_px', '8.0'),
        variable_get('ballads_viewer_mmpx_ratio', '8.0') * $matches[1]))
        . 'px';
  }
  else {
    return variable_get('ballads_viewer_mmpx_ratio', '8.0') * $matches[1] . 'px';
  }
}

/**
 * Convert a rend attribute to a CSS style.
 *
 * @param string $rend
 *   Contents of the rend attribute.
 *
 * @return string|null T
 *   The converted CSS style or null if the attribute is unparseable.
 */
function ballads_viewer_convert_rends($rend) {
  $matches = array();
  if (!preg_match('/^([a-z][a-z0-9-]*)/i', $rend, $matches)) {
    if (variable_get('ballads_viewer_debug', FALSE)) {
      drupal_set_message('unparseable rendering: ' . $rend, 'error');
    }
    else {
      error_log('unparseable rendering: ' . $rend);
    }
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
      return 'font-size: ' . ballads_viewer_convert_mm($rend, TRUE);

    case 'indent':
      return 'margin-left: ' . ballads_viewer_convert_mm($rend, FALSE);

    case 'length':
      return 'width: ' . ballads_viewer_convert_mm($rend, FALSE);

    default:
      if (variable_get('ballads_viewer_debug', FALSE)) {
        drupal_set_message('unknown rendering: ' . $rend, 'error');
      }
      else {
        error_log('unknown rendering: ' . $rend);
      }
      return NULL;
  }
}

/**
 * Find the appropriate render stylesheet for the object.
 *
 * If the object has a RENDER_STYLESHEET datastream, use it. If not, look at
 * the parent collections and use the first one found.
 *
 * @param IslandoraFedoraObject $islandora_object
 *   The object being displayed.
 *
 * @TODO will this have unpredictable results if the object has multiple
 * parents with different RENDER_STYLESHEET datastreams?
 */
function ballads_viewer_find_render_stylesheet(IslandoraFedoraObject $islandora_object) {
  if ($islandora_object['RENDER_STYLESHEET']) {
    return $islandora_object['RENDER_STYLESHEET']->content;
  }

  module_load_include('inc', 'islandora', 'includes/utilities');

  $parent_collections = islandora_get_parents_from_rels_ext($islandora_object);
  foreach ($parent_collections as $parent) {
    if ($parent['RENDER_STYLESHEET']) {
      return $parent['RENDER_STYLESHEET']->content;
    }
  }
  return NULL;
}

/**
 * Transform the XML with the XSL.
 *
 * @param string $xml_src
 *   The XML source for the transformation.
 * @param string $xsl_src
 *   The XSLT source to apply to the XML.
 *
 * @return DOMDocument
 *   The resulting DOM.
 */
function ballads_viewer_apply_stylesheet($xml_src, $xsl_src) {
  if ($xsl_src === NULL) {
    return NULL;
  }
  $xml_dom = new DOMDocument();
  $xml_dom->loadXML($xml_src);
  $xsl_dom = new DOMDocument();
  $xsl_dom->loadXML($xsl_src);

  $proc = new XSLTProcessor();
  $proc->importStylesheet($xsl_dom);
  $result = $proc->transformToDoc($xml_dom);
  return $result;
}

/**
 * Apply some post-processing to the transformed XML.
 *
 * Cleans up image URLs because the XSL doesn't know about islandora URLs and
 * converts @data-rend attributes to @style attributes with CSS.
 *
 * @param DOMDocument $xml_dom
 *   The HTML DOM to modify.
 * @param string $obj_id
 *   The Islandora Object ID being displayed.
 */
function ballads_viewer_postprocess(DOMDocument $xml_dom, $obj_id) {
  foreach ($xml_dom->getElementsByTagName('img') as $img) {
    $img->setAttribute('src', '/islandora/object/' . $obj_id . '/' . $img->getAttribute('src'));
  }

  // Fix nodes with TEI rend attributes.
  $xp = new DOMXpath($xml_dom);
  foreach ($xp->query('//*[@data-rend]') as $node) {
    $rend = $node->getAttribute('data-rend');
    $styles = array_map('ballads_viewer_convert_rends', preg_split('/\s+/', $rend));
    $node->setAttribute('style', implode('; ', $styles));
    $node->removeAttribute('data-rend');
  }

  // Fix the Table of Contents.
  $solr_base = variable_get('islandora_solr_url', 'http://localhost:8080/solr');
  foreach ($xp->query('//*[@class="toc-link"]') as $node) {
    $filename = $node->getAttribute('href');
    $result = file_get_contents("{$solr_base}/select?version=1.2&wt=json&json.nl=map&q=mods_identifier_original_filename_ms:%22{$filename}%22&rows=1&fl=PID&omitHeader=true");
    $json = json_decode($result, TRUE);
    if (isset($json['response']['docs'][0]['PID'])) {
      $node->setAttribute('href', "/islandora/object/" . $json['response']['docs'][0]['PID']);
    }
    else {
      $node->setAttribute('href', "#");
    }
  }
}

/**
 * Implements hook_preprocess_theme().
 */
function ballads_viewer_preprocess_ballads_viewer(array &$variables) {
  $path = drupal_get_path('module', 'ballads_viewer');
  drupal_add_css($path . '/css/ballads.css');
  $islandora_object = $variables['islandora_object'];

  $xml_src = $islandora_object['OBJ']->content;
  $xsl_src = ballads_viewer_find_render_stylesheet($islandora_object);
  if ($xsl_src === NULL) {
    drupal_set_message("No stylesheet found. Displaying the raw XML.", 'warning');
    $variables['ballads_viewer_content'] = '<pre>' . htmlspecialchars($xml_src) . '</pre>';
    return;
  }
  $result = ballads_viewer_apply_stylesheet($xml_src, $xsl_src);
  ballads_viewer_postprocess($result, $islandora_object->id);
  $variables['ballads_viewer_content'] = $result->saveXML();
  if (variable_get('ballads_viewer_linkxml', FALSE)) {
    $variables['ballads_viewer_linkxml'] = url('/islandora/object/' . $islandora_object->id . '/datastream/OBJ/view');
  }
  else {
    $variables['ballads_viewer_linkxml'] = FALSE;
  }
}
