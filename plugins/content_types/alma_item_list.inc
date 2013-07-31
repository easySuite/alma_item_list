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
  $page = $conf['alma_search_page'];
  $start = ($page - 1) * $limit;

  $result = alma_client_invoke('run_lms_search', $query, 'native', $start, $limit);

  module_load_include('ting', 'client');
  $objects = ting_get_objects($result['items']);

  $render = array();
  
  foreach ($objects as $identifier => $object) {
    if (is_object($object)) {
      $ding_entity = ding_entity_load($object->id);
      
      $loan_form = module_invoke('ding_reservation', 'ding_entity_buttons', 'ding_entity', $ding_entity, 'ajax');
      $cv = array(
        '#theme' => 'ting_object_cover',
        '#object' => $ding_entity,
        '#image_style' => 'item_list',
        '#attached' => array(
          'js' => array(drupal_get_path('module', 'ting_covers') . '/js/ting-covers.js'),
        ),
      );
      $render[] = theme('alma_item_list_item', array(
        'faust' => $ding_entity->ding_entity_id,
        'cover' => drupal_render($cv),
        'title' => $ding_entity->title,
        'author' => $ding_entity->creator,
        'year' => $ding_entity->date,
        'loan_form' => drupal_render($loan_form),
        'has_rating' => FALSE,
        'rating' => 0,
        'rating_count' => 0,
        'review_count' => 0,
      ));

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

  drupal_add_css(drupal_get_path('module', 'alma_item_list') . '/css/alma_item_list.css', 'file');
  $block = new stdClass();
  $block->content = theme('alma_item_list_list', array('items' => implode('', $render)));

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

  $form['alma_search_page'] = array(
    '#type' => 'textfield',
    '#title' => t('Page'),
    '#description' => t('Page offset from where to start the results. Wrong values can result in empty result set.'),
    '#default_value' => isset($conf['alma_search_page']) ? $conf['alma_search_page'] : '1',
    '#size' => 2,
    '#maxlength' => 3,
  );

  return $form;
}

/**
 * Custom validation handler for settings form.
 *
 * @see alma_item_list_search_content_type_edit_form().
 */
function alma_item_list_search_content_type_edit_form_validate($form, &$form_state) {
  $input = $form_state['input'];

  if (!is_numeric($input['alma_search_page']) || $input['alma_search_page'] < 1) {
    form_set_error('alma_search_page', t('The value must be a positive number'));
  }
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
  $form_state['conf']['alma_search_page'] = $input['alma_search_page'];
}
