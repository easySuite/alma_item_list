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
  'category' => t('easyOPAC'),
  'render callback' => 'alma_item_list_search_content_type_render',
  'admin title' => 'alma_item_list_search_content_type_admin_title',
  'edit form' => 'alma_item_list_search_content_type_edit_form',
);

/**
 * Return the pane contents.
 */
function alma_item_list_search_content_type_render($subtype, $conf, $panel_args, $context) {
  $params = implode(
    ',',
    array(
      $conf['alma_search_query'],
      $conf['alma_search_limit'],
      $conf['alma_search_cache'],
    )
  );
  $hash = md5($params);
  $markup = theme('alma_item_list_list', array('hash' => $hash));

  $block = new stdClass();
  $block->content['#markup'] = $markup;
  $path = drupal_get_path('module', 'alma_item_list');
  $block->content['#attached'] = array(
    'js' => array(
      array('data' => drupal_get_path('module', 'ding_availability') . '/js/ding_availability.js'),
      array('data' => drupal_get_path('module', 'ding_reservation') . '/js/ding_reservation.js'),
      array('data' => $path . '/js/alma_item_list.js'),
      array(
        'data' => array(
          $hash => array(
            'query' => $conf['alma_search_query'],
            'limit' => $conf['alma_search_limit'],
            'cache' => $conf['alma_search_cache'],
          ),
        ),
        'type' => 'setting'
      ),
    ),
    'css' => array(
      $path . '/css/alma_item_list.css',
    ),
    // Load required ajax for reservation, not get called if data is from cache.
    'library' => array(
      array('system', 'drupal.ajax'),
      array('system', 'jquery.form'),
    ),
  );

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
