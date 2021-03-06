<?php

/**
 * @file
 *   Adjustment on the file_entity admin interface.
 */

/**
 * Implements hook_form_FORM_ID_alter for file_entity_admin_files().
 *
 * Alter the media browser from to enable new UI
 */
function media_browser_plus_form_file_entity_admin_files_alter(&$form, &$form_state) {
  $path = drupal_get_path('module', 'media_browser_plus');

  // Add css for both views.
  $form['#attached']['library'][] = array('media_browser_plus', 'media_browser_plus');
  $form['#attached']['library'][] = array('media_browser_plus', 'colorbox');
  // Set base href for javascript requests.
  $settings = media_browser_plus_main_view_javascript_settings();

  $form['#attached']['js'][] = array(
    'type' => 'setting',
    'data' => $settings,
  );
  // Check which mode we are in.
  $menu_item = menu_get_item();
  if (!isset($menu_item['map'][3]) || $menu_item['map'][3] != 'thumbnails') {
    return _media_browser_plus_media_admin_list_alter($form, $form_state);
  }

  // Check access rights.
  if (!media_browser_plus_access('media grid view')) {
    drupal_access_denied();
    return array();
  }
  // Adding grid view js and css.
  $form['#attached']['js'][] = $path . '/js/media_browser_plus.admin.js';
  drupal_add_library('system', 'ui.draggable');
  drupal_add_library('system', 'ui.droppable');
  // Removing options form part.
  unset($form['options']);
  if (!media_browser_plus_access('media list view')
    || !media_browser_plus_access('media grid view')) {
    unset($form['switch']);
  }

  unset($form['pager']);
  $form['files'] = media_browser_plus_grid_view_form();
  $form['media-basket'] = media_browser_plus_media_basket_form();

  if (media_access('preview')) {
    $form['media-preview'] = media_browser_plus_media_preview_form();
  }
  return $form;
}

/**
 * Enter description here
 *
 * @param $form
 * @param $form_state
 */
function media_browser_plus_file_entity_admin_files_validate($form, &$form_state) {
  // Copy input files to values.
  if ($form_state['values']['op'] != 'Download') {
    $form_state['values']['files'] = $form_state['input']['files'];
    // Call the old validate.
    file_entity_admin_files_validate($form, $form_state);
  }
}

/**
 * Enter description here
 *
 * @param $form
 * @param $form_state
 */
function media_browser_plus_edit_multiple_redirect($form, &$form_state) {
  $fids = implode(' ', array_keys(array_filter($form_state['values']['files'])));
  $form_state['redirect'] = array(
    'admin/content/file/edit_multiple/' . $fids,
    array('query' => array('destination' => 'admin/content/file/thumbnails')),
  );
}

/**
 * Enter description here
 *
 * @param $form
 * @param $form_state
 */
function media_browser_plus_delete_multiple_redirect($form, &$form_state) {
  $fids = implode(' ', array_keys(array_filter($form_state['values']['files'])));
  $form_state['redirect'] = array(
    'admin/content/file/delete_multiple/' . $fids,
    array('query' => array('destination' => 'admin/content/file/thumbnails')),
  );
}

/**
 * Alter the media list display.
 *
 */
function _media_browser_plus_media_admin_list_alter(&$form, &$form_state) {
  global $user;

  // Check access rights.
  if (!media_browser_plus_access('media list view')) {
    drupal_access_denied();
    return array();
  }
  $header = array(
    'title' => array(
      'data' => t('Title'),
      'specifier' => 'filename',
      'type' => 'property',
      'field' => 'f.filename',
    ),
    'type' => array(
      'data' => t('Type'),
      'specifier' => 'filemime',
      'type' => 'property',
      'field' => 'f.filemime',
    ),
    'folder' => array(
      'data' => t('Folder'),
      'specifier' => array(
        'field' => 'field_folder',
        'column' => 'tid',
      ),
      'type' => 'field',
    ),
    'size' => array(
      'data' => t('Size'),
      'specifier' => 'filesize',
      'type' => 'property',
      'field' => 'f.filesize',
    ),
    'author' => array(
      'data' => t('Author'),
      'specifier' => 'uid',
      'type' => 'property',
      'field' => 'u.name',
    ),
    'timestamp' => array(
      'data' => t('Updated'),
      'specifier' => 'timestamp',
      'type' => 'property',
      'sort' => 'desc',
      'field' => 'f.timestamp',
    ),
    'operations' => array(
      'data' => t('Operations'),
    ),
  );
  $destination = drupal_get_destination();
  $options = array();
  $media_entities = media_browser_plus_load_multiple(array(
    'conditions' => array(),
    'header' => $header,
  ));
  // Gather the data from each media object.
  foreach ($media_entities->results as $media) {
    // Load folder name.
    $folder = '';
    if (isset($media->field_folder[LANGUAGE_NONE][0]['tid'])
    && ($term = taxonomy_term_load($media->field_folder[LANGUAGE_NONE][0]['tid']))) {
      $folder = $term->name;
    }

    // Load user object if media author is not the current user.
    if ($media->uid != $user->uid) {
      $media_user = user_load($media->uid);
    }
    // Media author is the same as current user no need to load the user.
    else{
      $media_user = $user;
    }

    $options[$media->fid] = array(
      'title' => theme('media_link', array('file' => $media)),
      'folder' => $folder,
      'type' => check_plain($media->filemime),
      'size' => format_size($media->filesize),
      'author' => theme('username', array('account' => $media_user)),
      'timestamp' => format_date($media->timestamp, 'short'),
    );
    $options[$media->fid]['operations'] = l(t('edit'), 'file/' . $media->fid . '/edit', array('query' => $destination));
  }
  // Grab old options.
  $ops = $form['options']['operation']['#options'];
  // Redo the options form.
  unset($form['options']);
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Operations'),
    '#weight' => -10,
    'operations' => array(
      '#type' => 'container',
      '#prefix' => '<div class="container-inline">',
      '#suffix' => '</div>',
      'operation' => array(
        '#type' => 'select',
        '#options' => $ops,
        '#default_value' => 'delete',
      ),
      'submit' => array(
        '#type' => 'submit',
        '#value' => t('Submit'),
        '#submit' => $form['#submit'],
      ),
    ),
    'folder' => array(
      '#type' => 'container',
      'selected_folder' => array(
        '#type' => 'select',
        '#title' => t('Media Folder*'),
        '#required' => FALSE,
        '#options' => _media_browser_plus_folder_list(),
      ),
      'submit' => array(
        '#type' => 'container',
        '#prefix' => '<div class="container-inline">',
        '#suffix' => '</div>',
        'submit' => array(
          '#type' => 'submit',
          '#value' => t('Change Folder'),
          '#submit' => array('media_browser_plus_media_admin_folder_change_submit'),
        ),
        'notice' => array(
          '#type' => 'item',
          '#markup' => t('*: change will be applied to ALL selected media files'),
        ),
      ),
    ),
  );
  $form['files']['#header'] = $header;
  $form['files']['#options'] = $options;

  // Reset pager.
  $form['pager'] = array('#markup' => _media_browser_plus_pager($media_entities->page, $media_entities->pages));

  return $form;
}

/**
 * Changes the folder of the submitted media items.
 */
function media_browser_plus_media_admin_folder_change_submit(&$form, &$form_state) {
  // Get IDs.
  $fids = array_keys(array_filter($form_state['values']['files']));
  $folder = (int) $form_state['values']['selected_folder'];

  $media_entities = file_load_multiple($fids);
  // Apply folder.
  foreach ($media_entities as $media) {
    if (isset($media->field_folder[LANGUAGE_NONE][0]['tid'])) {
      $media->field_folder[LANGUAGE_NONE][0]['tid'] = $folder;
    }
    else {
      $media->field_folder = array(LANGUAGE_NONE => array(array('tid' => $folder)));
    }
    media_browser_plus_move_file($folder, $media);
  }

  drupal_set_message(t('Folder changes applied successfully'));
}
