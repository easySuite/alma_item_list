<?php
/**
 * @file
 *
 * Wrapper template for item list.
 */
?>

<div class="alma-item-list"<?php if(!empty($hash)): ?> data-hash="<?php print $hash?>" <?php endif; ?>>
  <?php if (!empty($items)) : ?>
  <div class="alma-item-list-items">
    <?php print $items; ?>
  </div>
  <?php ;endif ?>
</div>
