<?php
/**
 * @file
 * Provides search results as a Panels content type.
 */

define('ALMA_ITEM_LIST_DEFAULT_CACHE_TTL', '86400');

$plugin = array(
  'title' => t('Alma search list'),
  'single' => TRUE,
  'description' => t('A list of the most recently acquired materials'),
  'category' => t('easyDDB'),
  'render callback' => 'alma_item_list_search_content_type_render',
  'admin title' => 'alma_item_list_search_content_type_admin_title',
  'edit form' => 'alma_item_list_search_content_type_edit_form',
);

/**
 * Return the pane contents.
 */
function alma_item_list_search_content_type_render($subtype, $conf, $panel_args, $context) {
  $query = $conf['alma_search_query'];
  $limit = $conf['alma_search_limit'];
  $page = 0;

  module_load_include('inc', 'ting', 'ting.client');
  module_load_include('inc', 'ting_covers', 'ting_covers.pages');

  $items = array();
  $render = array();

  $cid = 'alma_item_list-' . md5($query . $limit . $page);
  $cache_ttl = isset($conf['alma_search_cache']) ? $conf['alma_search_cache'] : ALMA_ITEM_LIST_DEFAULT_CACHE_TTL;
  $cache = cache_get($cid, 'cache');
  if (!empty($cache->data)) {
    $render = $cache->data;
  }
  else {
    try {
      $result = alma_client_invoke(
        'run_lms_search',
        $query,
        'native',
        0,
        200
      );
    }
    catch (AlmaClientHTTPError $e) {
      watchdog_exception('ding_item_list', $e);

      return (object) array('content' => '');
    }

    array_walk($result['items'], function (&$v, $k) {
      $v = '870970-basis:' . $v;
    });

    $objects = ting_get_objects($result['items']);
    $cover_data = array();
    $items_tmp = array();

    foreach ($objects as $object) {
      if (!is_object($object)) {
        continue;
      }
      $faust = explode(':', $object->id);
      $items_tmp[$faust[1]] = $object;

      $cover_data[] = array(
        'local_id' => $faust[1],
        'owner_id' => $faust[0],
        'image_style' => 'item_list',
      );
    }

    $covers = ting_covers_objects(FALSE, $cover_data);

    // Assign covers.
    foreach ($covers as $v) {
      if (!isset($v['urls']['detail'])) {
        continue;
      }

      $items[$v['local_id']] = array(
        'cover' => $v['urls']['detail'],
        'object' => $items_tmp[$v['local_id']],
      );
      // Exit search loop if we have reached limit.
      if (count($items) == $limit) {
        break;
      }
    }

    // Process items, load entities, voxb, etc.
    foreach ($items as $item) {
      $faust = explode(':', $item['object']->id);
      $ding_entity = ding_entity_load($item['object']->id);

      $reservation_form = module_invoke_all(
        'ding_entity_buttons',
        'ding_entity',
        $ding_entity,
        'ajax'
      );

      $render[$faust[1]] = array(
        'local_id' => $faust[1],
        'faust' => $ding_entity->ding_entity_id,
        'cover' => $item['cover'],
        'title' => $ding_entity->title,
        'author' => isset($ding_entity->creators[0]) ? $ding_entity->creators[0] : '',
        'year' => $ding_entity->date,
        'loan_form' => drupal_render($reservation_form),
        'has_rating' => FALSE,
        'rating' => 0,
        'rating_count' => 0,
        'review_count' => 0,
      );

      if (module_exists('ding_voxb') && isset($ding_entity->isbn[0])) {
        $isbn = $ding_entity->isbn[0];
        $voxb_details = new VoxbItems();
        $voxb_details->addReviewHandler('review', new VoxbReviews());
        $voxb_details->fetchByISBN(array($isbn));
        $detail = $voxb_details->getItem($isbn);

        if ($detail) {
          $voxb = array(
            'has_rating' => TRUE,
            'rating' => (int) round($detail->getRating() / 20),
            'rating_count' => $detail->getRatingCount(),
            'review_count' => $detail->getReviews('review')->getCount(),
          );
          $render[$faust[1]] += $voxb;
        }
      }
    }
    cache_set($cid, $render, 'cache', time() + $cache_ttl);
  }

  drupal_add_css(drupal_get_path('module', 'alma_item_list') . '/css/alma_item_list.css', 'file');

  // Load required ajax for reservation, not get called if data is from cache.
  drupal_add_library('system', 'drupal.ajax');
  drupal_add_library('system', 'jquery.form');
  drupal_add_js(drupal_get_path('module', 'ding_availability') . '/js/ding_availability.js');
  drupal_add_js(drupal_get_path('module', 'ding_reservation') . '/js/ding_reservation.js');

  // Generate markup.
  $content = array();
  foreach ($render as $v) {
    $content[] = theme('alma_item_list_item', $v);
  }
  $block_content = theme('alma_item_list_list', array('items' => implode('', $content)));

  $block = new stdClass();
  $block->content = $block_content;

  return $block;
}


/**
 * Return the pane titles in admin zone.
 */
function alma_item_list_search_content_type_admin_title($subtype, $conf, $context) {
  return t('Alma Item List');
}

/**
 * Returns an edit form for the content type.
 */
function alma_item_list_search_content_type_edit_form($form, &$form_state) {
  $conf = $form_state['conf'];

  $form['alma_search_query'] = array(
    '#type' => 'textfield',
    '#title' => t('Search query'),
    '#required' => TRUE,
    '#default_value' => isset($conf['alma_search_query']) ? $conf['alma_search_query'] : '',
  );

  $options = array_merge(range(1, 9, 1), range(10, 100, 10));
  $form['alma_search_limit'] = array(
    '#type' => 'select',
    '#title' => t('Number of items to search'),
    '#options' => array_combine($options, $options),
    '#default_value' => isset($conf['alma_search_limit']) ? $conf['alma_search_limit'] : '',
  );

  // Cache setting.
  $options = array(
    '86400' => t('24 hours'),
    '259200' => t('72 hours'),
    '604800' => t('1 week'),
    '18144000' => t('1 month'),
    '54432000' => t('3 months'),
  );

  $form['alma_search_cache'] = array(
    '#type' => 'select',
    '#title' => t('Cache'),
    '#options' => $options,
    '#default_value' => isset($conf['alma_search_cache']) ? $conf['alma_search_cache'] : ALMA_ITEM_LIST_DEFAULT_CACHE_TTL,
  );

  return $form;
}

/**
 * Custom submit handler for settings form.
 *
 * @see alma_item_list_search_content_type_edit_form()
 */
function alma_item_list_search_content_type_edit_form_submit($form, &$form_state) {
  $input = $form_state['input'];

  $form_state['conf']['alma_search_query'] = trim($input['alma_search_query']);
  $form_state['conf']['alma_search_limit'] = $input['alma_search_limit'];
  $form_state['conf']['alma_search_cache'] = $input['alma_search_cache'];
}
