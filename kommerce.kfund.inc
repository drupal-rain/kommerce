<?php

/**
 * Implements hook_commerce_payment_method_info().
 */
function kommerce_commerce_payment_method_info() {
  $payment_methods = array();

  $payment_methods['kfund_god'] = array(
    'title' => t('God'),
    'description' => t('Bypass any payment, it\'s paid by the God..'),
    'active' => FALSE,
  );

  $payment_methods['kfund_credit'] = array(
    'title' => t('Credit Wallet'),
    'short title' => t('Credit'),
    'description' => t('Pay with credit in your wallet.'),
    'active' => TRUE,
    'terminal' => FALSE, // If need to set this? It would allow multiple payment submit in terminal form.
  );

  return $payment_methods;
}

// -----------------------------------------------------------------------------
// kfund_credit callbacks

/**
 * Callback: BASE_settings_form().
 */
function kfund_credit_settings_form($settings = NULL) {
  // dsm($settings);
}

/**
 * Callback: BASE_submit_form().
 */
function kfund_credit_submit_form($payment_method, $pane_values, $checkout_pane, $order) {

  // Display Current Balance
  $credit_account = kfund_credit_account($order->uid);
  $form['balance'] = kfund_credit_account_balance_view($credit_account);
  $form['balance']['#prefix'] = '<div class="credit-account-balance">' . t('Current Balance: ');
  $form['balance']['#suffix'] = '</div>';

  $form['discount'] = array(
    '#type' => 'fieldset',
    '#title' => t('Discount'),
    '#tree' => TRUE,
  );
  $form['discount']['points'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use Points as Discount'),
  );
  $form['discount']['points_amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount'),
    '#states' => array(
      'visible' => array(
        ':input[name="commerce_payment[payment_details][discount][points]"]' => array('checked' => TRUE),
      ),
    ),
  );

  return $form;
}

/**
 * Callback: BASE_submit_form_validate().
 */
function kfund_credit_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {

  // Check Account Credit
  $credit_account_wrapper = kfund_credit_account_wrapper($order->uid);
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  $balance = $credit_account_wrapper->balance->value();
  $order_total = $order_wrapper->commerce_order_total->value();
  // @todo Check currency_code?
  if ($balance['amount'] < $order_total['amount']) {
    form_set_error('commerce_payment[payment_method]', t('Credit account balance is not enough to pay this order.'));
    return FALSE;
  }

  // @todo Check Account Point


  return TRUE;
}

/**
 * Callback: BASE_submit_form_submit().
 */
function kfund_credit_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Subtract the account balance by new transaction.
  $credit_account = kfund_credit_account($order->uid);
  kfund_credit_account_pay_order($credit_account, $order);
}

// -----------------------------------------------------------------------------
// Utility

/**
 * Get Commerce Marketplace Funds Account Entity of user.
 */
function kfund_credit_account($user) {
  $uid = is_object($user) ? $user->uid : $user;
  $cmp_fund_account = commerce_marketplace_funds_account_load_by_owner('user', $uid);

  return $cmp_fund_account;
}

/**
 * Get Commerce Marketplace Funds Account Entity Wrapper of user.
 */
function kfund_credit_account_wrapper($user) {
  $account = kfund_credit_account($user);

  return entity_metadata_wrapper('cmp_funds_account', $account);
}

// Generate a renderable array of credit account balance.
function kfund_credit_account_balance_view($account) {
  $balance_view = field_view_value('cmp_funds_account', $account, 'balance',
    $account->balance[LANGUAGE_NONE][0],
    array('lable' => 'hidden')
  );

  return $balance_view;
}

// Pay the order.
function kfund_credit_account_pay_order($account, $order) {
  // @todo Need to deal with commerce_payment_transaction?

  // @todo 'to_account' for store order.
  $transaction = (object) array(
    'transaction_id' => '',
    'type' => 'payment',
    'from_account' => $account->account_id,
    'to_account' => 0,//$store_account->account_id
    'order_id' => $order->order_id,
    'status' => FUNDS_TRANSACTION_SUCCESS,
  );
  $transaction->amount = $order->commerce_order_total;
  $order_amount = $order->commerce_order_total[LANGUAGE_NONE][0]['amount'];
  commerce_marketplace_funds_transaction_save($transaction);
  $account->balance[LANGUAGE_NONE][0]['amount'] -= $order_amount;
  commerce_marketplace_funds_account_save($account);
}
