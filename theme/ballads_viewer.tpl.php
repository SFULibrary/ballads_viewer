<?php
/**
 * @file
 * Theme template file for the Ballads Viewer.
 */
?>
<?php if($ballads_viewer_linkxml): ?>
  <a href="<?php echo $ballads_viewer_linkxml; ?>">XML Source</a>
<?php endif; ?>
<?php print $ballads_viewer_content; ?>
