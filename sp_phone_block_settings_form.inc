<?php

function sp_phone_block_general_settings_form($form, &$form_state) {

  $form['general_settings'] = array(
    '#type' => 'fieldset',
    '#title' => 'Algemene instellingen',
  );

  $form['general_settings']['sp_phone_block_campaign_name'] = array(
    '#type' => 'textfield',
    '#title' => 'Naam campagne:<br/>Ik wil niet meer gebeld worden door ...',
    '#description' => 'Vul de naam van de campagne in, inclusief lidwoord.',
    '#required' => TRUE,
    '#default_value' => variable_get('sp_phone_block_campaign_name'),
  );

  $form['general_settings']['sp_phone_block_max_attempts'] = array(
    '#type' => 'numberfield',
    '#title' => 'Maximaal aantal pogingen per uur',
    '#default_value' => variable_get('sp_phone_block_max_attempts',  3),
  );

  $form['email_settings'] = array(
    '#type' => 'fieldset',
    '#title' => 'E-mailinstellingen',
  );

  $form['email_settings']['sp_phone_block_email_subject'] = array(
    '#type' => 'textfield',
    '#title' => 'Onderwerp',
    '#default_value' => variable_get('sp_phone_block_email_subject'),
    '#required' => TRUE,
  );

  $default = variable_get('sp_phone_block_email_body',  array());
  $form['email_settings']['sp_phone_block_email_body'] = array(
    '#type' => 'text_format',
    '#title' => 'Inhoud',
    '#description' => 'De tekst [code] wordt vervangen door de bevestig code.',
    '#format' => isset($default['format']) ? $default['format'] : NULL,
    '#default_value' => isset($default['value']) ? $default['value'] : '',
    '#required' => TRUE,
  );

  $site_mail = variable_get('site_mail', ini_get('sendmail_from'));
  $form['email_settings']['sp_phone_block_email_from'] = array(
    '#type' => 'emailfield',
    '#title' => 'Afzend e-mailadres',
    '#default_value' => variable_get('sp_phone_block_email_from',  $site_mail),
    '#required' => TRUE,
  );

  $form['sms'] = array(
    '#type' => 'fieldset',
    '#title' => 'SMS instellingen',
  );

  $form['sms']['sp_phone_block_sms_url'] = array(
    '#type'          => 'textfield',
    '#title'         => 'API URL',
    '#default_value' => variable_get('sp_phone_block_sms_url', 'https://api.messagebird.com/xml/sms/'),
    '#description'   => 'API endpoint voor de SMS-dienst',
    '#required'      => true,
  );
 
  $form['sms']['sp_phone_block_sms_user'] = array(
    '#type'          => 'textfield',
    '#title'         => 'Gebruikersnaam',
    '#default_value' => variable_get('sp_phone_block_sms_user', 'systeembeheersp'),
    '#description'   => 'Gebruikersnaam voor de SMS-dienst',
    '#required'      => true,
  );

  $form['sms']['sp_phone_block_sms_pass'] = array(
    '#type'          => 'textfield',
    '#title'         => 'Wachtwoord',
    '#default_value' => variable_get('sp_phone_block_sms_pass', ''),
    '#description'   => 'Wachtwoord voor de SMS-dienst',
    '#required'      => true,
  );

  return system_settings_form($form);
}
