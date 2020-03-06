#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

#include "protocols/uip/uip.h"

#include "config.h"
#include "core/debug.h"
#include "core/bit-macros.h"

#include "mqtt_homie.h"

#ifdef ONEWIRE_HOMIE_SUPPORT
#include "hardware/onewire/onewire_homie.h"
#endif

#ifdef TANKLEVEL_HOMIE_SUPPORT
#include "services/tanklevel/tanklevel_homie.h"
#endif

#ifdef HEATCTL_HOMIE_SUPPORT
#include "services/heatctl/heatctl_homie.h"
#endif

typedef enum
{
  HOMIE_DISCONNECTED,
  HOMIE_INIT,
  HOMIE_DEVICE_META,
  HOMIE_NODE_META,
  HOMIE_READY,
  HOMIE_ACTIVE
} mqtt_homie_state_t;

void mqtt_homie_init(void);

static bool send_device_meta_str_P(PGM_P attr, PGM_P value);
static bool send_device_meta_localip(void);
static bool send_device_meta_mac(void);
static bool send_device_meta_nodes(void);
static bool send_device_meta(void);
static bool send_device_state(const char *state);

static bool send_node_meta_str_P(PGM_P node_id, PGM_P attr, PGM_P value);
static bool send_node_meta_str_ptr(PGM_P node_id, PGM_P attr, PGM_P const *value);
static bool send_node_meta_props(PGM_P node_id, const mqtt_homie_property_t * const *props);
static bool send_node_meta(const mqtt_homie_node_t *node);
static bool send_nodes_meta(void);

static int8_t get_prop_array_count(const mqtt_homie_property_t *prop);
static bool header_prop_attr(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr);

static bool send_prop_meta_str_P(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr, PGM_P value);
static bool send_prop_meta_str_ptr(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr, PGM_P const *value);
static bool send_prop_meta_settable(PGM_P node_id, PGM_P prop_id, int8_t array_idx, input_callback_t const *input_callback);
static bool send_prop_meta_datatype(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr, uint8_t const *value);
static bool send_prop_meta_cb_fixed(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr, mqtt_homie_bool_s8_callback_t const *cb, PGM_P const *fixed);
static bool send_prop_subscribe(PGM_P node_id, PGM_P prop_id, int8_t array_idx, input_callback_t const *input_callback);
static bool send_prop_meta_array(const mqtt_homie_node_t *node, const mqtt_homie_property_t *prop);
static bool send_prop_meta(const mqtt_homie_node_t *node, const mqtt_homie_property_t *prop, int8_t array_idx);
static bool send_props_meta(const mqtt_homie_node_t *node);

static bool call_prop_output_callback(const mqtt_homie_property_t *prop, int8_t array_idx);
static bool call_prop_output_callback_array(const mqtt_homie_property_t *prop);
static bool call_node_output_callback(const mqtt_homie_node_t *node);
static bool call_nodes_output_callback(void);

static void set_state(mqtt_homie_state_t new_state);

static const char *match_topic(const char *data, const char *end, PGM_P prefix, char sep);

static void connack_cb(void);
static void poll_cb(void);
static void close_cb(void);
static void publish_cb(char const *topic, uint16_t topic_length, const void *payload, uint16_t payload_length, bool retained);

static const mqtt_callback_config_t mqtt_homie_callback_config PROGMEM = {
  .connack_callback = connack_cb,
  .poll_callback = poll_cb,
  .close_callback = close_cb,
  .publish_callback = publish_cb
};

static const char str_empty[] PROGMEM = "";
static const char str_comma[] PROGMEM = ",";
static const char str_array[] PROGMEM = "[]";
static const char str_homie[] PROGMEM = "homie";

static const char dev_id[] PROGMEM = MQTT_HOMIE_CONF_DEV_ID;

static const char fmt_S_S_S[] PROGMEM = "%S/%S/%S";
static const char fmt_S_S_S_S[] PROGMEM = "%S/%S/%S/%S";
static const char fmt_S_S_S_S_S[] PROGMEM = "%S/%S/%S/%S/%S";
static const char fmt_S_S_S_S_d_S[] PROGMEM = "%S/%S/%S/%S_%d/%S";
static const char fmt_S_S_S_S_d[] PROGMEM = "%S/%S/%S/%S_%d";

static const mqtt_homie_node_t * const nodes[] PROGMEM =
{
#ifdef ONEWIRE_HOMIE_SUPPORT
  &ow_homie_node,
#endif
#ifdef TANKLEVEL_HOMIE_SUPPORT
  &tanklevel_homie_node,
#endif
#ifdef HEATCTL_HOMIE_SUPPORT
  &heatctl_homie_node,
#endif
  NULL
};

static mqtt_homie_state_t state;
static uint8_t dev_field;
static uint8_t node_idx;
static uint8_t node_field;
static uint8_t prop_idx;
static uint8_t prop_field;
static int8_t prop_array_count;
static int8_t prop_array_idx;

static bool
send_device_meta_str_P(PGM_P attr, PGM_P value)
{
  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S, str_homie, dev_id, attr);
  mqtt_construct_publish_packet_payload(PSTR("%S"), value);
  return mqtt_construct_publish_packet_fin();
}

static bool
send_device_meta_localip(void)
{
  uip_ipaddr_t hostaddr;
  uip_gethostaddr(&hostaddr);

  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S, str_homie, dev_id, PSTR("$localip"));
#if UIP_CONF_IPV6
  mqtt_construct_publish_packet_payload(
    PSTR("%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x"),
    HTONS(hostaddr[0]), HTONS(hostaddr[1]),
    HTONS(hostaddr[2]), HTONS(hostaddr[3]),
    HTONS(hostaddr[4]), HTONS(hostaddr[5]),
    HTONS(hostaddr[6]), HTONS(hostaddr[7]));
#else
  mqtt_construct_publish_packet_payload(
    PSTR("%u.%u.%u.%u"),
    HI8(HTONS(hostaddr[0])), LO8(HTONS(hostaddr[0])),
    HI8(HTONS(hostaddr[1])), LO8(HTONS(hostaddr[1])));
#endif
  return mqtt_construct_publish_packet_fin();
}

static bool
send_device_meta_mac(void)
{
  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S, str_homie, dev_id, PSTR("$mac"));
  mqtt_construct_publish_packet_payload(PSTR("%02X:%02X:%02X:%02X:%02X:%02X"),
    uip_ethaddr.addr[0], uip_ethaddr.addr[1], uip_ethaddr.addr[2],
    uip_ethaddr.addr[3], uip_ethaddr.addr[4], uip_ethaddr.addr[5]);
  return mqtt_construct_publish_packet_fin();
}

static bool
send_device_meta_nodes(void)
{
  bool first;
  const mqtt_homie_node_t * const *np;
  const mqtt_homie_node_t *n;
  const char *node_id;

  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S, str_homie, dev_id, PSTR("$nodes"));
  for (first = true, np = nodes; (n = (const mqtt_homie_node_t *) pgm_read_word(np)) != NULL; np++) {
    node_id = (const char *) pgm_read_word(&n->id);
    mqtt_construct_publish_packet_payload(PSTR("%S%S"),
      first ? str_empty : str_comma, node_id);
    first = false;
  }
  return mqtt_construct_publish_packet_fin();
}

static bool
send_device_meta(void)
{
  switch (dev_field)
  {
    case 0:
      if (!send_device_meta_str_P(PSTR("$homie"), PSTR("3.0.1")))
        return false;
      dev_field++;

    case 1:
      if (!send_device_meta_str_P(PSTR("$name"), PSTR(MQTT_HOMIE_CONF_DEV_NAME)))
        return false;
      dev_field++;

    case 2:
      if (!send_device_meta_localip())
        return false;
      dev_field++;

    case 3:
      if (!send_device_meta_mac())
        return false;
      dev_field++;

    case 4:
      if (!send_device_meta_nodes())
        return false;
      dev_field++;

    case 5:
      if (!send_device_meta_str_P(PSTR("$implementation"), PSTR("Ethersex")))
        return false;
      dev_field++;
  }

  return true;
}

static bool
send_device_state(const char *state)
{
  return send_device_meta_str_P(PSTR("$state"), state);
}

static bool
send_node_meta_str_P(PGM_P node_id, PGM_P attr, PGM_P value)
{
  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S, str_homie, dev_id, node_id, attr);
  mqtt_construct_publish_packet_payload(PSTR("%S"), value);
  return mqtt_construct_publish_packet_fin();
}

static bool
send_node_meta_str_ptr(PGM_P node_id, PGM_P attr, PGM_P const *value)
{
  PGM_P s = (PGM_P) pgm_read_word(value);
  if (s == NULL)
    return true;
  return send_node_meta_str_P(node_id, attr, s);
}

static bool
send_node_meta_props(PGM_P node_id, const mqtt_homie_property_t * const *props)
{
  const mqtt_homie_property_t *prop = (const mqtt_homie_property_t *) pgm_read_word(props);
  bool first;
  const char *prop_id;
  int8_t i, array_count;

  mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S, str_homie, dev_id, node_id, PSTR("$properties"));
  for (first = true; (prop_id = (const char *) pgm_read_word(&prop->id)) != NULL; prop++) {
    array_count = get_prop_array_count(prop);
    if (array_count < 0)
    {
      mqtt_construct_publish_packet_payload(PSTR("%S%S"),
        first ? str_empty : str_comma, prop_id);
      first = false;
    } else {
      for (i = 0; i < array_count; i++)
      {
        mqtt_construct_publish_packet_payload(PSTR("%S%S_%d"),
          first ? str_empty : str_comma, prop_id, i);
        first = false;
      }
    }
  }
  return mqtt_construct_publish_packet_fin();
}

static bool
send_node_meta(const mqtt_homie_node_t *node)
{
  PGM_P node_id = (PGM_P) pgm_read_word(&node->id);
  mqtt_homie_bool_void_callback_t init_cb;
  switch (node_field)
  {
    case 0:
      init_cb = (mqtt_homie_bool_void_callback_t) pgm_read_word(&node->init_callback);
      if (init_cb != NULL)
      {
        if (!init_cb())
          return false;
      }
      node_field++;

    case 1:
      if (!send_node_meta_str_ptr(node_id, PSTR("$name"), &node->name))
        return false;
      node_field++;

    case 2:
      if (!send_node_meta_str_ptr(node_id, PSTR("$type"), &node->type))
        return false;
      node_field++;

    case 3:
      if (!send_node_meta_props(node_id, &node->properties))
        return false;
      node_field++;
  }

  return true;
}

static bool
send_nodes_meta(void)
{
  while(1)
  {
    const mqtt_homie_node_t *node = (const mqtt_homie_node_t *) pgm_read_word(&nodes[node_idx]);
    if (node == NULL)
      return true;

    if (!send_node_meta(node))
      return false;

    if (!send_props_meta(node))
      return false;

    node_idx++;
    node_field = 0;
    prop_idx = 0;
    prop_field = 0;
  }
}

static int8_t
get_prop_array_count(const mqtt_homie_property_t *prop)
{
  mqtt_homie_s8_void_callback_t callback = (mqtt_homie_s8_void_callback_t) pgm_read_word(&prop->array_count_callback);
  if (callback == NULL)
    return HOMIE_ARRAY_FLAG_NOARR;
  return callback();
}

static bool
header_prop_attr(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr)
{
  if (array_idx >= 0)
  {
    return mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S_d_S, str_homie, dev_id, node_id, prop_id, array_idx, attr);
  } else {
    return mqtt_construct_publish_packet_header(1, true, fmt_S_S_S_S_S, str_homie, dev_id, node_id, prop_id, attr);
  }
}

static bool
send_prop_meta_str_P(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr, PGM_P value)
{
  header_prop_attr(node_id, prop_id, array_idx, attr);
  mqtt_construct_publish_packet_payload(PSTR("%S"), value);
  return mqtt_construct_publish_packet_fin();
}

static bool
send_prop_meta_str_ptr(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr, PGM_P const *value)
{
  PGM_P s = (PGM_P) pgm_read_word(value);
  if (s == NULL)
    return true;
  return send_prop_meta_str_P(node_id, prop_id, array_idx, attr, s);
}

static bool
send_prop_meta_settable(PGM_P node_id, PGM_P prop_id, int8_t array_idx, input_callback_t const *input_callback)
{
  input_callback_t cb = (input_callback_t) pgm_read_word(input_callback);
  return send_prop_meta_str_P(node_id, prop_id, array_idx, PSTR("$settable"), mqtt_homie_bool(cb != NULL));
}

static bool
send_prop_meta_datatype(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr, uint8_t const *value)
{
  mqtt_homie_datatype_t dt = (mqtt_homie_datatype_t) pgm_read_byte(value);
  PGM_P s;
  switch(dt)
  {
    case HOMIE_DATATYPE_STRING:
      s = PSTR("string");
      break;
    case HOMIE_DATATYPE_INTEGER:
      s = PSTR("integer");
      break;
    case HOMIE_DATATYPE_FLOAT:
      s = PSTR("float");
      break;
    case HOMIE_DATATYPE_BOOL:
      s = PSTR("boolean");
      break;
    case HOMIE_DATATYPE_ENUM:
      s = PSTR("enum");
      break;
    case HOMIE_DATATYPE_COLOR:
      s = PSTR("color");
      break;
  }
  return send_prop_meta_str_P(node_id, prop_id, array_idx, attr, s);
}

static bool
send_prop_meta_cb_fixed(PGM_P node_id, PGM_P prop_id, int8_t array_idx, PGM_P attr, mqtt_homie_bool_s8_callback_t const *cb, PGM_P const *fixed)
{
  mqtt_homie_bool_s8_callback_t callback = (mqtt_homie_bool_s8_callback_t) pgm_read_word(cb);
  if (callback != NULL)
  {
    header_prop_attr(node_id, prop_id, array_idx, attr);
    callback(array_idx);
    return mqtt_construct_publish_packet_fin();
  }

  return send_prop_meta_str_ptr(node_id, prop_id, array_idx, attr, fixed);
}

static bool
send_prop_subscribe(PGM_P node_id, PGM_P prop_id, int8_t array_idx, input_callback_t const *input_callback)
{
  input_callback_t cb = (input_callback_t) pgm_read_word(input_callback);
  if (cb == NULL)
    return true;

  if (array_idx >= 0)
  {
    return mqtt_construct_subscribe_packet_P(fmt_S_S_S_S_d, str_homie, dev_id, node_id, prop_id, array_idx);
  } else {
    return mqtt_construct_subscribe_packet_P(fmt_S_S_S_S, str_homie, dev_id, node_id, prop_id);
  }
}

static bool
send_prop_meta(const mqtt_homie_node_t *node, const mqtt_homie_property_t *prop, int8_t array_idx)
{
  PGM_P node_id = (PGM_P) pgm_read_word(&node->id);
  PGM_P prop_id = (PGM_P) pgm_read_word(&prop->id);

  switch (prop_field)
  {
    case 0:
      if (!send_prop_meta_cb_fixed(node_id, prop_id, array_idx, PSTR("$name"), &prop->name_callback, &prop->name))
        return false;
      prop_field++;

    case 1:
      if (!send_prop_meta_settable(node_id, prop_id, array_idx, &prop->input_callback))
        return false;
      prop_field++;

    case 2:
      if (!send_prop_meta_str_ptr(node_id, prop_id, array_idx, PSTR("$unit"), &prop->unit))
        return false;
      prop_field++;

    case 3:
      if (!send_prop_meta_datatype(node_id, prop_id, array_idx, PSTR("$datatype"), &prop->datatype))
        return false;
      prop_field++;

    case 4:
      if (!send_prop_meta_cb_fixed(node_id, prop_id, array_idx, PSTR("$format"), &prop->format_callback, &prop->format))
        return false;
      prop_field++;

    case 5:
      if (!send_prop_subscribe(node_id, prop_id, array_idx, &prop->input_callback))
        return false;
      prop_field++;
  }

  prop_field = 0;
  return true;
}

static bool
send_prop_meta_array(const mqtt_homie_node_t *node, const mqtt_homie_property_t *prop)
{
  while (prop_array_idx < prop_array_count)
  {
    if (!send_prop_meta(node, prop, prop_array_idx))
      return false;

    prop_array_idx++;
    prop_field = 0;
  }

  return true;
}

static bool
send_props_meta(const mqtt_homie_node_t *node)
{
  while(1)
  {
    const mqtt_homie_property_t *props = (const mqtt_homie_property_t *) pgm_read_word(&node->properties);
    const mqtt_homie_property_t *prop = &props[prop_idx];
    PGM_P prop_id = (PGM_P) pgm_read_word(&prop->id);
    if (prop_id == NULL)
      return true;

    if (prop_array_count == HOMIE_ARRAY_FLAG_INIT)
      prop_array_count = get_prop_array_count(prop);

    if (prop_array_count >= 0)
    {
      if (!send_prop_meta_array(node, prop))
        return false;
    } else {
      if (!send_prop_meta(node, prop, HOMIE_ARRAY_FLAG_NOARR))
        return false;
    }

    prop_idx++;
    prop_field = 0;
    prop_array_count = HOMIE_ARRAY_FLAG_INIT;
    prop_array_idx = 0;
  }
}

static bool
call_prop_output_callback(const mqtt_homie_property_t *prop, int8_t array_idx)
{
    mqtt_homie_bool_s8_callback_t callback = (mqtt_homie_bool_s8_callback_t) pgm_read_word(&prop->output_callback);
    if (callback != NULL)
    {
      if (!callback(array_idx))
        return false;
    }

    return true;
}

static bool
call_prop_output_callback_array(const mqtt_homie_property_t *prop)
{
  while (prop_array_idx < prop_array_count)
  {
    if (!call_prop_output_callback(prop, prop_array_idx))
      return false;

    prop_array_idx++;
    prop_field = 0;
  }

  return true;
}


static bool
call_node_output_callback(const mqtt_homie_node_t *node)
{
  while(1)
  {
    const mqtt_homie_property_t *props = (const mqtt_homie_property_t *) pgm_read_word(&node->properties);
    const mqtt_homie_property_t *prop = &props[prop_idx];

    PGM_P prop_id = (PGM_P) pgm_read_word(&prop->id);
    if (prop_id == NULL)
      return true;

    if (prop_array_count == HOMIE_ARRAY_FLAG_INIT)
      prop_array_count = get_prop_array_count(prop);

    if (prop_array_count >= 0)
    {
      if (!call_prop_output_callback_array(prop))
        return false;
    } else {
      if (!call_prop_output_callback(prop, HOMIE_ARRAY_FLAG_NOARR))
        return false;
    }

    prop_idx++;
    prop_array_count = HOMIE_ARRAY_FLAG_INIT;
    prop_array_idx = 0;
  }
}

static bool
call_nodes_output_callback(void)
{
  while(1)
  {
    const mqtt_homie_node_t *node = (const mqtt_homie_node_t *) pgm_read_word(&nodes[node_idx]);
    if (node == NULL)
      return true;

    if (!call_node_output_callback(node))
      return false;

    node_idx++;
    prop_idx = 0;
  }
}

static void
set_state(mqtt_homie_state_t new_state)
{
  state = new_state;
  dev_field = 0;
  node_idx = 0;
  node_field = 0;
  prop_idx = 0;
  prop_field = 0;
  prop_array_count = HOMIE_ARRAY_FLAG_INIT;
  prop_array_idx = 0;
}

void
mqtt_homie_init(void)
{
  set_state(HOMIE_DISCONNECTED);
  mqtt_register_callback(&mqtt_homie_callback_config);
}

static void
connack_cb(void)
{
  set_state(HOMIE_INIT);
}

static void
poll_cb(void)
{
  switch (state)
  {
    case HOMIE_DISCONNECTED:
      break;
    case HOMIE_INIT:
      if (send_device_state(PSTR("init")))
        set_state(HOMIE_DEVICE_META);
      break;
    case HOMIE_DEVICE_META:
      if (send_device_meta())
        set_state(HOMIE_NODE_META);
      break;
    case HOMIE_NODE_META:
      if (send_nodes_meta())
        set_state(HOMIE_READY);
      break;
    case HOMIE_READY:
      if (send_device_state(PSTR("ready")))
        set_state(HOMIE_ACTIVE);
      break;
    case HOMIE_ACTIVE:
      if (call_nodes_output_callback())
        set_state(HOMIE_ACTIVE);
      break;
  }
}

static void
close_cb(void)
{
  set_state(HOMIE_DISCONNECTED);
}

static const char *
match_topic(const char *data, const char *end, PGM_P prefix, char sep)
{
  char c;
  for(; (c = (char) pgm_read_byte(prefix)) != 0; prefix++)
  {
    if (data >= end || *data != c)
      return NULL;
    data++;
  }

  if (sep == 0)
  {
    if (data < end)
      return NULL;
  } else {
    if (data >= end || *data != sep)
      return NULL;
    data++;
  }

  return data;
}

static void
publish_cb(char const *topic, uint16_t topic_length, const void *payload, uint16_t payload_length, bool retained)
{
  char const *topic_end = topic + topic_length;
  const char *node_in = match_topic(topic, topic_end, str_homie, '/');
  if (node_in == NULL)
    return;
  node_in = match_topic(node_in, topic_end, dev_id, '/');
  if (node_in == NULL)
    return;

  const mqtt_homie_node_t * const *np;
  const mqtt_homie_node_t *node;
  for (np = nodes; (node = (const mqtt_homie_node_t *) pgm_read_word(np)) != NULL; np++)
  {
    PGM_P node_id = (PGM_P) pgm_read_word(&node->id);

    const char *prop_in = match_topic(node_in, topic_end, node_id, '/');
    if (prop_in == NULL)
      continue;

    const mqtt_homie_property_t *prop;
    PGM_P prop_id;
    for (prop = (const mqtt_homie_property_t *) pgm_read_word(&node->properties); (prop_id = (PGM_P) pgm_read_word(&prop->id)) != NULL; prop++)
    {
      int8_t array_count = get_prop_array_count(prop);
      int8_t array_idx;
      if (array_count >= 0)
      {
        const char *array_idx_in = match_topic(prop_in, topic_end, prop_id, '_');
        if (array_idx_in == NULL)
          continue;
        array_idx = atoi(array_idx_in);
        if (array_idx < 0 || array_idx >= array_count)
          continue;
      } else {
        if (match_topic(prop_in, topic_end, prop_id, 0) == NULL)
          continue;
        array_idx = HOMIE_ARRAY_FLAG_NOARR;
      }

      input_callback_t cb = (input_callback_t) pgm_read_word(&prop->input_callback);
      if (cb != NULL)
        cb(array_idx, (const char *) payload, payload_length, retained);
    }
  }
}

bool
mqtt_homie_header_prop_value(PGM_P node_id, PGM_P prop_id, int8_t array_idx, bool retain)
{
  if (array_idx >= 0)
  {
    return mqtt_construct_publish_packet_header(1, retain, fmt_S_S_S_S_d, str_homie, dev_id, node_id, prop_id, array_idx);
  } else {
    return mqtt_construct_publish_packet_header(1, retain, fmt_S_S_S_S, str_homie, dev_id, node_id, prop_id);
  }
}

PGM_P
mqtt_homie_bool(bool value)
{
  return value ? PSTR("true") : PSTR("false");
}

/*
  -- Ethersex META --
  header(protocols/mqtt/mqtt_homie.h)
  init(mqtt_homie_init)
*/
