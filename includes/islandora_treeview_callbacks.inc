<?php
/**
 * @file
 * Contains all callbacks for treeview module.
 */

/**
 * Creates HTML to represent Fedora objects.
 *
 * @return string
 *   HMTL string to be displayed in file viewer.
 */
function islandora_treeview_sync() {
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora', 'includes/mime.detect');
  $mime_helper = new MimeDetect();
  $_POST['dir'] = urldecode($_POST['dir']);
  $collection_pid = check_plain(urldecode($_POST['pid']));
  // Remove hash.
  $collection_pid = preg_replace('/#$/', '', $collection_pid);
  $path = check_plain(urldecode($_POST['dir']));
  if ($path != '/') {
    $path = rtrim($path, '/');
  }
  if (!$collection_pid |!islandora_is_valid_pid($collection_pid)) {
    return;
  }
  $collection_object = islandora_object_load($collection_pid);
  if (!$collection_object |!isset($collection_object)) {
    echo(t("The collection @collection doesn't exist in this Fedora Repository.", array('@collection' => $collection_pid)));
    return;
  }
  $query = islandora_treeview_get_collection_query($collection_pid);
  $objects = $collection_object->repository->ri->sparqlQuery($query, 'unlimited');
  $dirstring = '';
  $filestring = '';

  foreach ($objects as $object) {
    $rel = htmlentities($path . $object['label']['value']);
    $label = $object['label']['value'];
    $filepid = $object['object']['value'];
    if ($object['model']['value'] == 'islandora:collectionCModel') {
      $dirstring .= "<li class='directory collapsed'><a href='#' rel='$rel' pid = '$filepid'>$label</a></li>";
    }
    else {
      $mime_query = islandora_treeview_get_mimetype_query($object['object']['value']);
      $mimetypes = $collection_object->repository->ri->sparqlQuery($mime_query, 'unlimited');
      $mime_type = $mimetypes[0]['object']['value'];
      $ext = $mime_helper->getExtension($mime_type);

      $filestring .= "<li title = '$mime_type' class='file ext_$ext dropbox_file_item'><a href='#' class = 'dropbox_file_item' rel='$rel'  pid='$filepid'>$label</a></li>";
    }
  }
  echo "<ul class='jqueryFileTree' style='display: none;'>$dirstring $filestring</ul>";
}

/**
 * Setup function for treeview.
 *   Creates the empty div to be populated by the javascript.
 *
 * @param type $fileview_basedir
 *  Optional directory to serve as root of displayed collections.
 *
 * @return string
 *   HTML with marker text.
 *
 */
function islandora_treeview_filetree($fileview_basedir = NULL) {
  $path = drupal_get_path('module', 'islandora_treeview');
  if (!$fileview_basedir) {
    $fileview_basedir = variable_get('islandora_repository_pid', 'islandora:root');
  }
  drupal_add_js("$path/js/jqueryFileTree.js");
  drupal_add_js(array('islandora_fileview' => array('fileview_basedir' => $fileview_basedir)), 'setting');
  drupal_add_css("$path/css/islandora_treeview.css");

  $output = "<div id='fileview'>Content will go here</div>";
  return $output;
}

/**
 * Creates custom SPARQL query.
 *  Query will retreice all child objects of $collection_pid.
 * @param string $collection_pid
 *   PID to query.
 *
 * @return string
 *   SPARQL query.
 */
function islandora_treeview_get_collection_query($collection_pid) {
  $query = <<<EOQ
            PREFIX fm: <info:fedora/fedora-system:def/model#>
            PREFIX fr: <info:fedora/fedora-system:def/relations-external#>
            SELECT ?object ?model ?label
            FROM <#ri>
            WHERE {
              {?object fm:hasModel ?model;
                       fr:isMemberOfCollection <info:fedora/$collection_pid>;
              }
            UNION
                    {?object fm:hasModel ?model;
                       fr:isMemberOf <info:fedora/$collection_pid>;
              }
            ?object fm:state fm:Active;
            FILTER (?model != <info:fedora/fedora-system:FedoraObject-3.0>)
            OPTIONAL{
                 ?object fm:label ?label
            }
           }
EOQ;
  return $query;
}

/**
 * Creates custom SPARQL query.
 *  Will retreive mimetype of object.
 * @param string $pid
 *   PID to query.
 *
 * @return string
 *   SPARQL query.
 */
function islandora_treeview_get_mimetype_query($pid) {
  $query = <<<EOQ
            PREFIX fm: <info:fedora/fedora-system:def/view#>
            SELECT ?object
            FROM <#ri>
            WHERE {
               <info:fedora/$pid/OBJ> fm:mimeType ?object
            }
EOQ;
  return $query;
}

