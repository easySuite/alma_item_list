<?php
// $Id$

/**
 * @file result.inc
 * Provides search results as a Panels content type.
 */

$plugin = array(
  'title' => t('Alma search list'),
  'single' => TRUE,
  'description' => t('A list of the most recently acquired materials'),
  'category' => t('Ting'),
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

  module_load_include('ting', 'client');
  module_load_include('inc', 'ting_covers', 'ting_covers.pages');

  $render = array();
  $cover_data = array();
  $ting_objects = array();

  while (count($render) != $limit) {
    $start = $page * $limit;
    $result = alma_client_invoke('run_lms_search', $query, 'native', $start, $limit);

    if ($result['status'] == 'error' || $result['numrecordsTotal'] == 0 || $result['numRecords'] == 0) {
      break;
    }

    $objects = ting_get_objects($result['items']);

    foreach ($objects as $object) {
      if (is_object($object)) {
        $ting_objects[] = $object;
        $ding_entity = ding_entity_load($object->id);

        $loan_form = module_invoke('ding_reservation', 'ding_entity_buttons', 'ding_entity', $ding_entity, 'ajax');
        $faust = explode(':', $ding_entity->ding_entity_id);

        $render[$faust[1]] = array(
          'local_id' => $faust[1],
          'faust' => $ding_entity->ding_entity_id,
          'cover' => '',
          'title' => $ding_entity->title,
          'author' => $ding_entity->creator,
          'year' => $ding_entity->date,
          'loan_form' => drupal_render($loan_form),
          'has_rating' => FALSE,
          'rating' => 0,
          'rating_count' => 0,
          'review_count' => 0,
        );

        $cover_data[] = array(
          'local_id' => $faust[1],
          'owner_id' => $faust[0],
          'image_style' => 'item_list',
        );

        if (module_exists('ding_voxb') && isset($ding_entity->isbn[0])) {
          $isbn = $ding_entity->isbn[0];
          $voxb_details = new VoxbItems();
          $voxb_details->addReviewHandler('review', new VoxbReviews());
          $response = $voxb_details->fetchByISBN(array($isbn));
          $detail = $voxb_details->getItem($isbn);

          if ($detail) {
            $voxb = array(
              'has_rating' => TRUE,
              'rating' => (int) round($detail->getRating() / 20),
              'rating_count' => $detail->getRatingCount(),
              'review_count' => $detail->getReviews('review')->getCount(),
            );

            $render += $voxb;
          }
        }
      }
    }

    $covers = ting_covers_objects(FALSE, $cover_data);

    // Assign covers.
    foreach ($covers as $v) {
      $render[$v['local_id']]['cover'] = $v['url'];
    }

    // Cleanup items w/o covers.
    foreach ($render as $k => $v) {
      if (empty($v['cover'])) {
        unset($render[$k]);
      }
    }

    $page++;
  }

  // Generate markup.
  $content = array();
  foreach ($render as $v) {
    $content[] = theme('alma_item_list_item', $v);
  }

  drupal_add_css(drupal_get_path('module', 'alma_item_list') . '/css/alma_item_list.css', 'file');

  $block = new stdClass();
  $block->content = theme('alma_item_list_list', array('items' => implode('', $content)));

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

  return $form;
}

/**
 * Custom submit handler for settings form.
 *
 * @see alma_item_list_search_content_type_edit_form().
 */
function alma_item_list_search_content_type_edit_form_submit($form, &$form_state) {
  $input = $form_state['input'];

  $form_state['conf']['alma_search_query'] = check_plain(trim($input['alma_search_query']));
  $form_state['conf']['alma_search_limit'] = $input['alma_search_limit'];
}
