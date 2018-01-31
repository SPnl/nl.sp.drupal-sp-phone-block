<?php

function sp_phone_block_form($form, &$form_state) {

  $form = array();

  if (!empty($form_state['storage'])) {
    $storage = &$form_state['storage'];
    //$form['test'] = array(
    //  '#markup' => '<pre>'.print_r($storage, TRUE).'</pre>',
    //);
  }

  $form['#prefix'] = '<div id="sp-phone-block-ajax-form">';
  $form['#suffix'] = '</div>';

  $form['#attached']['css'] = array(
    drupal_get_path('module', 'sp_phone_block') . '/sp_phone_block.css',
  );

  $form['#attributes'] = array(
    'class' => array('sp-phone-block-form'),
  );


  // Check if module is configured.
  if (empty(variable_get('sp_phone_block_sms_pass', ''))) {
    $form['feedback'] = array(
      '#markup' => '<div class="messages error">Configureer de module: <a href="/admin/config/sp/phone_block">instellingen</a>.</div>',
    );
    return $form;
  }

  // Check if access limit is reached.
  if (sp_phone_block_check_attempt_limit() > variable_get('sp_phone_block_max_attempts', 0)) {
    $form['feedback'] = array(
      '#markup' => '<div class="messages error"><p>U heeft het maximale aantal pogingen bereikt, probeert u het over enige tijd nogmaals.</p></div>',
    );
    return $form;
  }

  if (!empty($storage['message'])) {
    $form['message'] = array(
      '#markup' => '<div class="messages ' . $storage['message']['type'] . '">' . $storage['message']['value'] . '</div>',
    );
    unset($storage['message']);
  }

  // Get phonenumber.
  if (empty($storage['accounts'])) {
    $campaign_name = variable_get('sp_phone_block_campaign_name', ' ... ');
    $form['intro'] = array(
      '#markup' => '<p>Wilt u niet meer gebeld worden door ' . $campaign_name . ', vul dan hieronder uw telefoonnummer in.</p>',
    );

    $form['phonenumber'] = array(
      '#type' => 'telfield',
      '#title' => 'Telefoonnummer',
      '#size' => 10,
    );

    $form['submit_phone'] = array(
      '#type' => 'submit',
      '#value' => 'Indienen',
    );
  }

  // Get email.
  if (
    isset($storage['send_code_to']) &&
    $storage['send_code_to'] === 'email' &&
    empty($storage['code_send'])
  ) {
    $form['intro'] = array(
      '#markup' => '<p>Om uit schrijven sturen wij u een e-mail met daarin een bevestigingscode. Naar welk e-mailadres mogen we deze code sturen?</p>',
    );

    $form['email'] = array(
      '#type' => 'emailfield',
      '#title' => 'E-mailadres',
    );

    $form['submit_email'] = array(
      '#type' => 'submit',
      '#value' => 'Verder',
    );
  }

  // Check code.
  if (
    isset($storage['code_send']) &&
    empty($storage['code_ok'])
  ) {
    $trans = array(
      'mobile' => 'sms',
      'email' => 'email',
    );

    $form['intro'] = array(
	    '#markup' => '<p>We hebben u zojuist per ' . $trans[$storage['send_code_to']] . ' een code toegestuurd.</p><p>Vul deze hieronder in om uw nummer te laten blokkeren.</p>',
    );

    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => 'Code',
    );

    $form['submit_code'] = array(
      '#type' => 'submit',
      '#value' => 'Verder',
    );
  }

  // Code ok feedback.
  if (!empty($storage['code_ok'])) {
    $form['feedback'] = array(
      '#markup' => '<p>Uw telefoonnummer is geblokkeerd, u zal niet meer gebeld worden.</p>',
    );
  }

  // Add ajax to submit buttons.
  foreach ($form as $key => $form_array) {
    if (isset($form_array['#type']) && $form_array['#type'] === 'submit') {
      $form[$key]['#ajax'] = array(
        'wrapper' => 'sp-phone-block-ajax-form',
        'callback' => 'sp_phone_block_form_ajax_callback',
      );
      $form[$key]['#name'] = $key;
    }
  }
  return $form;
}

function sp_phone_block_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $storage = &$form_state['storage'];

  if (!empty($form_state['triggering_element']['#name'])) {
    $trigger = $form_state['triggering_element']['#name'];
    switch ($trigger) {
    case 'submit_phone':
      $values['phonenumber'] = trim($values['phonenumber']);
      $regex = '#(^\+[0-9]{2}|^\+[0-9]{2}\(0\)|^\(\+[0-9]{2}\)\(0\)|^00[0-9]{2}|^0)([0-9]{9}$|[0-9\-\s]{10}$)#';
      if (!preg_match($regex, $values['phonenumber'])) {
        form_set_error('phonenumber', 'Vul een geldig Nederlands telefoonnummer in.');
      }
      break;
    case 'submit_email':
      $values['email'] = trim($values['email']);

      // Check email.
      if (valid_email_address($values['email'])) {
        $email_errors = sp_validate_email($values['email']);
        if (!empty($email_errors)) {
          $mail_error = TRUE;
        }
      }
      else {
        $mail_error = TRUE;
      }
      if (!empty($mail_error)) {
        form_set_error('email', 'Vul een geldig e-mail adres in. Als het adres geldig is, maar desondanks niet geaccepteerd wordt, neem dan contact op met webmaster@sp.nl.');
      }
      elseif (sp_phone_block_email_blocked_number($values['email']) > 2) {
        form_set_error('email', 'Er is een fout opgetreden, neem contact op met webmaster@sp.nl om u af te melden voor het bellen.');
        $from = variable_get('sp_phone_block_email_from',  $site_mail);
        $params = $storage;
        global $base_url;
        $params['site'] = $base_url;
        $params['email'] = $values['email'];
        drupal_mail('sp_phone_block', 'max_block', 'webmaster@sp.nl', language_default(), $params, $from);
      }
      break;
    case 'submit_code':
      $values['code'] = trim($values['code']);
      if (!preg_match('/^[0-9]{4}$/', $values['code'])) {
        form_set_error('code', 'Vul een geldige code in');
      }
      break;
    }
  }
}

function sp_phone_block_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $storage = &$form_state['storage'];

  if (!empty($form_state['triggering_element']['#name'])) {
    $trigger = $form_state['triggering_element']['#name'];
    switch ($trigger) {
    case 'submit_phone':
      // Store phonenumber.
      $phoneclean = preg_replace('/[^0-9]/', '', $values['phonenumber']);
      $phoneclean = '0' . substr($phoneclean, -9);
      $storage['phonenumber'] = $phoneclean;
      // Get CiviCRM accounts.
      $accounts = sp_phone_block_get_civi_accounts($storage['phonenumber']);
      if (!$accounts) {
        sp_phone_block_store_attempt($storage['phonenumber']);
        $storage['message'] = array(
          'value' => 'Het telefoonnummer kon niet worden gevonden.',
          'type' => 'error',
        );
      }
      else {
        // Store accounts.
        $storage['accounts'] = $accounts;
        // Create code.
        $code = mt_rand(1000, 9999);
        $storage['code'] = $code;

        // Check if mobile phone.
        if (substr($phoneclean, -9, 1) === '6') {
          // Mobile phone.
          $storage['send_code_to'] = 'mobile';
          // Send code.
          $result = sp_phone_block_send_code_sms($code, $storage['phonenumber']);
          if ($result === FALSE) {
            $storage['send_code_to'] = 'email';
          }
          else {
            $storage['code_send'] = TRUE;
          }
        }
        else {
          // Email.
          $storage['send_code_to'] = 'email';
        }
      }
      break;
    case 'submit_email':
      $storage['email'] = $values['email'];
      $storage['code_send'] = TRUE;

      $site_mail = variable_get('site_mail', ini_get('sendmail_from'));
      $from = variable_get('sp_phone_block_email_from',  $site_mail);
      $subject = variable_get('sp_phone_block_email_subject', 'Uw afmeldcode');
      $body_raw = variable_get('sp_phone_block_email_body', array('value' => 'Uw afmeldcode is: [code]', 'format' => 'filtered_html'));
      $body_formatted = check_markup($body_raw['value'], $body_raw['format']);
      $body = str_replace('[code]', $storage['code'], $body_formatted);
      $params = array(
        'subject' => check_plain($subject),
        'body' => $body,
      );
      $result = drupal_mail('sp_phone_block', 'code_message', $storage['email'], language_default(), $params, $from);
      if ($result['result']) {
        $storage['code_send'] = TRUE;
        watchdog('sp_phone_block', 'E-mail met code is verzonden aan: ' . $storage['email']);
      }
      else {
        $storage['message'] = array(
          'value' => 'Er is iets misgegaan bij het verzenden van de e-mail.',
          'type' => 'error',
        );
        watchdog('sp_phone_block', 'Er is een fout opgetreden bij het versturen van de e-mail met code aan: ' . $storage['email']);
      }
      break;
    case 'submit_code':
      if ($values['code'] == $storage['code']) {
        if (sp_phone_block_block_phone($storage['accounts'])) {
          $email = empty($storage['email']) ? '' : $storage['email'];
          sp_phone_block_log($storage['accounts'], $storage['phonenumber'], $email);
          $storage['code_ok'] = TRUE;
        }
        else {
          $storage['message'] = array(
            'value' => 'Het is niet gelukt om de telefoonblokkade in te stellen. Probeert u het op een later moment nog eens. Lukt het dan nog steeds niet, neem dan contact op met webmaster@sp.nl.',
            'type' => 'error',
          );
        }
      }
      else {
        sp_phone_block_store_attempt($storage['phonenumber']);
        $storage['message'] = array(
          'value' => 'De code is niet correct!',
          'type' => 'error',
        );
      }
      break;
    }
  }
  $form_state['rebuild'] = TRUE;
}

function sp_phone_block_form_ajax_callback($form, &$form_state) {
  return $form;
}
