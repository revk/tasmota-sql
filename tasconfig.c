// Tasmota SQL config

#include <stdio.h>
#include <string.h>
#include <popt.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ctype.h>
#include <err.h>
#include <sqllib.h>
#include <ajl.h>
#include <mosquitto.h>

int debug = 0;

int main(int argc, const char *argv[])
{
   const char *sqlhostname = NULL;
   const char *sqldatabase = "tasmota";
   const char *sqlusername = NULL;
   const char *sqlpassword = NULL;
   const char *sqlconffile = NULL;
   const char *sqltable = "device";
   const char *mqtthostname = "mqtt";
   const char *mqttusername = NULL;
   const char *mqttpassword = NULL;
   const char *mqttid = NULL;
   int all = 0;
   int info = 0;

   poptContext optCon;          // context for parsing command-line options
   const struct poptOption optionsTable[] = {
      { "sql-conffile", 'c', POPT_ARG_STRING, &sqlconffile, 0, "SQL conf file", "filename" },
      { "sql-hostname", 'H', POPT_ARG_STRING, &sqlhostname, 0, "SQL hostname", "hostname" },
      { "sql-database", 'd', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &sqldatabase, 0, "SQL database", "db" },
      { "sql-username", 'U', POPT_ARG_STRING, &sqlusername, 0, "SQL username", "name" },
      { "sql-password", 'P', POPT_ARG_STRING, &sqlpassword, 0, "SQL password", "pass" },
      { "sql-table", 't', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &sqltable, 0, "SQL table", "table" },
      { "sql-debug", 'v', POPT_ARG_NONE, &sqldebug, 0, "SQL Debug" },
      { "mqtt-hostname", 'h', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &mqtthostname, 0, "MQTT hostname", "hostname" },
      { "mqtt-username", 'u', POPT_ARG_STRING, &mqttusername, 0, "MQTT username", "username" },
      { "mqtt-password", 'p', POPT_ARG_STRING, &mqttpassword, 0, "MQTT password", "password" },
      { "mqtt-id", 0, POPT_ARG_STRING, &mqttid, 0, "MQTT id", "id" },
      { "info", 0, POPT_ARG_NONE, &info, 0, "Show changes", NULL },
      { "all", 0, POPT_ARG_NONE, &all, 0, "All devices", NULL },
      POPT_AUTOHELP { }
   };

   optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
   poptSetOtherOptionHelp(optCon, "{topics}");

   int c;
   if ((c = poptGetNextOpt(optCon)) < -1)
      errx(1, "%s: %s\n", poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(c));

   if (poptPeekArg(optCon) ? all : !all)
   {
      poptPrintUsage(optCon, stderr, 0);
      return -1;
   }

   SQL sql;
   const char *topic = NULL;    // Current topic / device
   char *foundtopic = NULL;
   char *foundpayload = NULL;
   int foundpayloadlen = 0;
   int e = mosquitto_lib_init();
   if (e)
      errx(1, "MQTT init failed %s", mosquitto_strerror(e));
   struct mosquitto *mqtt = mosquitto_new(mqttid, 1, NULL);
   if (mqttusername)
   {
      e = mosquitto_username_pw_set(mqtt, mqttusername, mqttpassword);
      if (e)
         errx(1, "MQTT auth failed %s", mosquitto_strerror(e));
   }
   void connect(struct mosquitto *mqtt, void *obj, int rc) {
      obj = obj;
      rc = rc;
      e = mosquitto_subscribe(mqtt, NULL, "stat/#", 0);
      if (e)
         errx(1, "MQTT subscribe failed %s", mosquitto_strerror(e));
   }
   void disconnect(struct mosquitto *mqtt, void *obj, int rc) {
      obj = obj;
      rc = rc;
   }
   void message(struct mosquitto *mqtt, void *obj, const struct mosquitto_message *msg) {
      obj = obj;
      if (foundtopic)
         return;
      char *s = strchr(msg->topic, '/');
      if (!s)
         return;
      s++;
      if (strncmp(s, topic, strlen(topic)))
         return;
      s += strlen(topic);
      if (*s != '/')
         return;
      if (sqldebug)
         warnx("%s: %.*s", msg->topic, msg->payloadlen, (char *) msg->payload ? : "");
      foundtopic = strdup(msg->topic);
      if (!foundtopic)
         errx(1, "malloc");
      foundpayloadlen = msg->payloadlen;
      foundpayload = malloc(foundpayloadlen + 1);
      if (!foundpayload)
         errx(1, "malloc");
      if (foundpayloadlen)
         memcpy(foundpayload, msg->payload, foundpayloadlen);
      foundpayload[foundpayloadlen] = 0;
   }
   mosquitto_connect_callback_set(mqtt, connect);
   mosquitto_disconnect_callback_set(mqtt, disconnect);
   mosquitto_message_callback_set(mqtt, message);
   e = mosquitto_connect(mqtt, mqtthostname, 1883, 60);
   if (e)
      errx(1, "MQTT connect failed (%s) %s", mqtthostname, mosquitto_strerror(e));
   sql_real_connect(&sql, sqlhostname, sqlusername, sqlpassword, sqldatabase, 0, NULL, 0, 1, sqlconffile);

   int getstat(void) {          // Wait for a message from current topic device...
      free(foundtopic);
      foundtopic = NULL;
      free(foundpayload);
      foundpayload = NULL;
      struct timeval now;
      gettimeofday(&now, NULL);
      long long giveup = now.tv_sec * 1000000 + now.tv_usec + 1000000;
      while (1)
      {
         e = mosquitto_loop(mqtt, 1000, 1);
         if (e)
            errx(1, "MQTT loop failed %s", mosquitto_strerror(e));
         if (e)
            return e;
         if (foundtopic)
            return 0;
         gettimeofday(&now, NULL);
         if (now.tv_sec * 1000000 + now.tv_usec > giveup)
            break;
      }
      return -1;
   }

   void config(SQL_RES * res) { // Configure a devices
      if (sqldebug)
         warnx("Config for %s", topic);
      else if (info)
         fprintf(stderr, "Checking %s\n", topic);
      // Check device is on line even...
      char *t = NULL;
      if (asprintf(&t, "cmnd/%s/status", topic) < 0)
         errx(1, "malloc");
      int e = mosquitto_publish(mqtt, NULL, t, 0, NULL, 0, 0);
      if (e)
         errx(1, "MQTT publish failed %s", mosquitto_strerror(e));
      free(t);
      if (getstat())
      {
         char *alt = sql_colz(res, "_Topic");
         if (alt && *alt)
         {
            warnx("Not responding %s, trying %s", topic, alt);
            topic = alt;
            if (asprintf(&t, "cmnd/%s/status", topic) < 0)
               errx(1, "malloc");
            int e = mosquitto_publish(mqtt, NULL, t, 0, NULL, 0, 0);
            if (e)
               errx(1, "MQTT publish failed %s", mosquitto_strerror(e));
            free(t);
            if (getstat())
            {
               warnx("Not responding %s", topic);
               return;
            }
         } else
         {
            warnx("Not responding %s (and no alternative to try)", topic);
            return;
         }
      }
      // Check settings
      const char *name = NULL,
          *value = NULL;
      for (int n = 0; n < res->field_count; n++)
         if ((value = res->current_row[n]) && *(name = res->fields[n].name) != '_')
         {
            char *t = NULL;
            if (asprintf(&t, "cmnd/%s/%s", topic, name) < 0)
               errx(1, "malloc");
            int e = mosquitto_publish(mqtt, NULL, t, 0, NULL, 0, 0);
            if (e)
               errx(1, "publish failed %s", mosquitto_strerror(e));
            free(t);
            if (getstat())
            {
               warnx("Not data for %s/%s", topic, name);
               continue;
            }
            char match = 0;
            const char *v = "unknown";
            j_t j = j_create();
            const char *je = j_read_mem(j, foundpayload, foundpayloadlen);
            if (je)
               warnx("Bad JSON: %s (%s)", foundpayload, je);
            else
            {
               v = NULL;
               if (!strncmp(name, "Rule", 4) && isdigit(name[4]))
               {
                  j_t r = j_find(j, name);
                  if (r)
                     v = j_get(r, "Rules");
               } else
                  v = j_get(j, name);
               if (!v)
                  warnx("Not found %s in (%s)", name, foundpayload);
               else if (!strcmp(v, value))
                  match = 1;
            }
            if (!match)
            {                   // Send
               if (asprintf(&t, "cmnd/%s/%s", topic, name) < 0)
                  errx(1, "malloc");
               int e = mosquitto_publish(mqtt, NULL, t, strlen(value), value, 0, 0);
               if (e)
                  errx(1, "publish failed %s", mosquitto_strerror(e));
               free(t);
               if (getstat())
                  warnx("No response setting %s to %s on %s", name, value, topic);
               else
               {
                  j_t j2 = j_create();
                  const char *je = j_read_mem(j2, foundpayload, foundpayloadlen);
                  if (je)
                     warnx("Bad JSON: %s (%s)", foundpayload, je);
                  else
                  {
                     const char *v2 = NULL;
                     if (!strncmp(name, "Rule", 4) && isdigit(name[4]))
                     {
                        j_t r = j_find(j2, name);
                        if (r)
                           v2 = j_get(r, "Rules");
                     } else
                        v2 = j_get(j2, name);
                     if (!v2)
                        warnx("Not found %s in (%s)", name, topic);
                     else if (strcmp(v2, value))
                        warnx("Unable to set %s to %s on %s (is %s)", name, value, topic, v2);
                     else
                     {
                        if (info)
                           fprintf(stderr, "Updated %s to %s on %s (was %s)\n", name, value, topic, v);
                        if (!strncmp(name, "Rule", 4) && isdigit(name[4]) && strcmp(value, "0"))
                        {       // Special case for Rule setting
                           if (asprintf(&t, "cmnd/%s/%s", topic, name) < 0)
                              errx(1, "malloc");
                           int e = mosquitto_publish(mqtt, NULL, t, 1, "1", 0, 0);
                           if (e)
                              errx(1, "publish failed %s", mosquitto_strerror(e));
                           free(t);
                           if (getstat())
                              warnx("Setting %s 1 did not work on %s", name, topic);
                        }
                     }
                  }
                  j_delete(&j2);
               }
            }
            j_delete(&j);
         }
   }

   if (all)
   {                            // All
      SQL_RES *res = sql_safe_query_store_free(&sql, sql_printf("SELECT * FROM `%#S`", sqltable));
      while (sql_fetch_row(res))
      {
         topic = sql_colz(res, "Topic");
         config(res);
      }
      sql_free_result(res);
   }
   {                            // Args
      while ((topic = poptGetArg(optCon)))
      {
         SQL_RES *res = sql_safe_query_store_free(&sql, sql_printf("SELECT * FROM `%#S` WHERE `Topic`=%#s", sqltable, topic));
         if (sql_fetch_row(res))
            config(res);
         else
            warnx("Not found in %s table: %s", sqltable, topic);
         sql_free_result(res);
      }
   }
   // Tidy up
   free(foundtopic);
   free(foundpayload);
   mosquitto_destroy(mqtt);
   mosquitto_lib_cleanup();
   sql_close(&sql);
   poptFreeContext(optCon);
   return 0;
}
