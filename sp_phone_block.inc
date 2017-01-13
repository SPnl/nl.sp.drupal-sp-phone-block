<?php

/*
 * Functions for sp_phone_block_module.
 */

function sp_phone_block_check_attempt_limit() {
  $ip = ip_address();
  $query = new entityFieldQuery();
  $result = $query
    ->entityCondition('entity_type', 'attempt')
    ->propertyCondition('ip', $ip)
    ->propertyCondition('time', time() - 3600, '>')
    ->count()
    ->execute();

  return $result;
}

function sp_phone_block_get_civi_accounts($phonenumber) {
  $phonenumber = preg_replace('/[^0-9]/', '', $phonenumber);
  $phonepart = substr($phonenumber, -9);
  $spcivi = \SPCivi::getInstance();
  $params = array(
    'option.limit'          => 9999,
    'phone_numeric' => array('LIKE' => '%' . $phonepart),
  );
  $data = array();
  $results = $spcivi->api('Phone', 'get', $params);
  if (sp_phone_block_check_result($results, 'contacten met telefoonnummer', 'get', $params)) {
    if (!empty($results['values'])) {
      foreach ($results['values'] as $value) {
        $data[] = $value['contact_id'];
      }
      return $data;
    }
  }
  return FALSE;
}

function sp_phone_block_check_result($result, $dataname, $action, $params = NULL, $debug_info = NULL) {
  switch ($action) {
  case 'get':
    $action_name = 'ophalen';
    $place_name = 'uit';
    break;
  case 'create':
    $action_name = 'aanmaken';
    $place_name = 'in';
    break;
  case 'update':
    $action_name = 'bijwerken';
    $place_name = 'in';
    break;
  }
  if (!isset($result['is_error']) || $result['is_error'] == 1) {
    drupal_set_message('Fout bij het ' . $action_name . ' van ' . $dataname . ' ' . $place_name . ' CiviCRM.', 'error');
    watchdog('sp_phone_block', '<p>Fout bij het ' . $action_name . ' van ' . $dataname . ' ' . $place_name . ' CiviCRM.</p>Resultaat:<pre>' . print_r($result, TRUE) . '</pre>Parameters:<pre>' . print_r($params, TRUE) . '</pre>Debug info:<pre>' . print_r($debug_info, TRUE) . '</pre>', array(), WATCHDOG_ERROR);
    return FALSE;
  }
  return TRUE;
}

function sp_phone_block_store_attempt($phonenumber) {
  $attempt = entity_create('attempt', array(
    'type' => 'phone',
    'value' => $phonenumber,
    'time' => time(),
    'ip' => ip_address(),
  ));
  $result = entity_save('attempt', $attempt);
  return $result;
}

function sp_phone_block_send_code_sms($code, $mobile) {

  $url = variable_get('sp_phone_block_sms_url');
  $smsuser = variable_get('sp_phone_block_sms_user');
  $smspass = variable_get('sp_phone_block_sms_pass');

  if (!$url || !$smsuser || !$smspass) {
    drupal_set_message('Sms provider is niet ingesteld.', 'error');
    watchdog('sptwofactor', 'Sms provide has not been configured.', NULL, WATCHDOG_WARNING);
    return false;
  }

  $params = "username=" . $smsuser . "&md5_password=" . $smspass . "&originator=SPnet&recipients=" . $mobile . "&message=Je afmeldcode is: " . $code . "&gateway=1";
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, $url);
  curl_setopt($ch, CURLOPT_POST, true);
  curl_setopt($ch, CURLOPT_POSTFIELDS, $params);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
  $output = simplexml_load_string(curl_exec($ch));
  curl_close($ch);

  switch ($output->item->resultcode) {
    case 10:
      watchdog('sp_phone_block', 'Sent SMS to ' . $mobile, NULL, WATCHDOG_DEBUG);
      return true;
      break;
    default:
      drupal_set_message("Er is een fout opgetreden bij het versturen van een sms. Resultaatcode was " . $output->item->resultcode . " - " . $output->item->resultmessage, 'error');
      watchdog('sp_phone_block', 'Could not send SMS to ' . $mobile . '! Error ' . $output->item->resultcode . ' - ' . $output->item->resultmessage, NULL, WATCHDOG_WARNING);
      return false;
  }
}

/*
 * Set CiviCRM privacy settings for contact to no calls.
 */
function sp_phone_block_block_phone($accounts) {
  set_time_limit(0);
  $spcivi = \SPCivi::getInstance();

  foreach ($accounts as $contact_id) { 
    $params = array(
      'id' => $contact_id,
      'do_not_phone' => 1,
    );
    $result = $spcivi->api('Contact', 'create', $params);
    if (sp_phone_block_check_result($result, 'een contact', 'update', $params)) {
      watchdog('sp_phone_block', 'CiviCRM contact nr. '.$contact_id.' privacy instelling "Niet bellen" ingeschakeld.');
    }
    else {
      return FALSE;
    }
  }
  return TRUE;
}
