// Tasmota SQL config

#include <stdio.h>
#include <string.h>
#include <popt.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
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
   const char *setting = NULL;
   int all = 0;
   int info = 0;
   int backup = 0;

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
      { "setting", 0, POPT_ARG_STRING, &setting, 0, "Only this setting", "setting" },
      { "info", 0, POPT_ARG_NONE, &info, 0, "Show changes", NULL },
      { "backup", 0, POPT_ARG_NONE, &backup, 0, "Backup device", NULL },
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
   char *name = NULL;
   char *value = NULL;
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
      if ( all)
      {
         e = mosquitto_subscribe(mqtt, NULL, "tele/#", 0);
         if (e)
            errx(1, "MQTT subscribe failed %s", mosquitto_strerror(e));
      }
   }
   void disconnect(struct mosquitto *mqtt, void *obj, int rc) {
      obj = obj;
      rc = rc;
   }
   struct found_s {
      struct found_s *next;
      char *topic;
   } *found = NULL;
   int finding = all;
   void message(struct mosquitto *mqtt, void *obj, const struct mosquitto_message *msg) {
      if (sqldebug)
         warnx("< %s %.*s", msg->topic, msg->payloadlen, (char *) msg->payload ? : "");
      obj = obj;
      // Expecting stat/[name]/RESULT {"name"...
      // Check topic
      char *s = msg->topic;
      if (finding && !strncmp(s, "tele/", 5))
      {                         // Add to list
         char *t = s + 5;
         char *d = strchr(t, '/');
         if (d && !strcmp(d, "/LWT") && msg->payloadlen == 6 && !strncmp((char *) msg->payload, "Online", 6))
         {
            *d = 0;
            struct found_s *f = malloc(sizeof(*f));
            f->topic = strdup(t);
            f->next = found;
            found = f;
         }
      }
      if (foundtopic || !name)
         return;
      if (strncmp(s, "stat/", 5))
         return;
      s += 5;
      if (strncmp(s, topic, strlen(topic)))
         return;
      s += strlen(topic);
      if (*s != '/')
         return;
      s++;
      if (strcmp(s, "RESULT"))
         return;
      // Check payload
      j_t j = j_create();
      const char *je = j_read_mem(j, (char *) msg->payload, msg->payloadlen);
      if (!je && !j_find(j, name))
         je = "nope";
      j_delete(&j);
      if (je)
         return;
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
   e = mosquitto_loop_start(mqtt);
   if (e)
      errx(1, "MQTT loop failed (%s) %s", mqtthostname, mosquitto_strerror(e));

   sql_real_connect(&sql, sqlhostname, sqlusername, sqlpassword, sqldatabase, 0, NULL, 0, 1, sqlconffile);

   void sendfree(char *topic, int len, void *payload) {
      free(foundtopic);
      foundtopic = NULL;
      free(foundpayload);
      foundpayload = NULL;
      if (sqldebug)
         warnx("> %s %.*s", topic, len, (char *) payload ? : "");
      int e = mosquitto_publish(mqtt, NULL, topic, len, payload, 0, 0);
      if (e)
         errx(1, "MQTT publish failed %s", mosquitto_strerror(e));
      free(topic);
   }

   int getstat(void) {          // Wait for a message from current topic device...
      struct timeval now;
      gettimeofday(&now, NULL);
      long long giveup = now.tv_sec * 1000000 + now.tv_usec + 2000000;
      while (1)
      {
         usleep(10000);
         if (foundtopic)
            return 0;
         gettimeofday(&now, NULL);
         if (now.tv_sec * 1000000 + now.tv_usec > giveup)
            break;
      }
      return -1;
   }

   void config(SQL_RES * res) { // Configure a devices
      // Check device is on line even...
      name = "Topic";
      char *t = NULL;
      if (asprintf(&t, "cmnd/%s/%s", topic, name) < 0)
         errx(1, "malloc");
      sendfree(t, 0, NULL);
      if (getstat())
      {
         char *alt = sql_colz(res, "_Topic");
         if (alt && *alt)
         {
            warnx("Not responding %s, trying %s", topic, alt);
            topic = alt;
            if (asprintf(&t, "cmnd/%s/status", topic) < 0)
               errx(1, "malloc");
            sendfree(t, 0, NULL);
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
      if (sqldebug)
         warnx("Config for %s", topic);
      else if (info)
         fprintf(stderr, "Checking %s\n", topic);
      // Check settings
      const char *findval(j_t j) {      // Get value, including special cases e.g. for rules
         j_t r = j_find(j, name);
         const char *v = j_val(r);
         if (!strncmp(name, "Rule", 4) && isdigit(name[4]))
         {
            if (v)
               return j_get(j, "Rules");        // Alternative style...
            if (r)
               return j_get(r, "Rules");
            return NULL;
         }
         if (j_isobject(r) && (!strcmp(name, "Sleep") || !strcmp(name, "Module")))
            return j_name(j_first(r));  // Stupid format
         if (!strncmp(name, "PulseTime", 9) && isdigit(name[9]))
         {
            if (r)
               return j_get(r, "Set");
            return NULL;
         }
         if (!v)
            return NULL;
         if (!strcmp(name, "Webserver"))
         {
            if (strstr(v, "Active for Admin"))
               return "2";
            if (strstr(v, "Active for User"))
               return "1";
            return "0";
         }
         if (!strncmp(name, "SetOption", 9))
         {
            if (!strcmp(v, "ON"))
               return "1";
            if (!strcmp(v, "OFF"))
               return "0";
            return v;
         }
         return v;
      }
      sql_string_t s = { };
      if (backup)
         sql_sprintf(&s, "UPDATE `%#S` SET ", sqltable);
      for (int n = 0; n < res->field_count; n++)
         if ((!setting || !strcasecmp(res->fields[n].name, setting)) && ((value = res->current_row[n]) || backup) && *(name = res->fields[n].name) != '_')
         {
            char *t = NULL;
            if (asprintf(&t, "cmnd/%s/%s", topic, name) < 0)
               errx(1, "malloc");
            sendfree(t, 0, NULL);
            if (getstat())
            {
               warnx("No data for %s/%s", topic, name);
               continue;
            }
            char match = 0;
            const char *v = NULL;
            j_t j = j_create();
            const char *je = j_read_mem(j, foundpayload, foundpayloadlen);
            if (je)
               warnx("Bad JSON: %s (%s)", foundpayload, je);
            else
            {
               v = findval(j);
               if (!v)
                  warnx("Not found %s in (%s)", name, foundpayload);
               else if ((!value || !*value) && (!v || !*v))
                  match = 1;
               else if (v && value && !strcmp(v, value))
                  match = 1;
            }
            if (!match)
            {                   // Send / store
               if (backup)
               {                // Store
                  if (strncmp(name, "Password", 9) || !v || strcmp(v, "****"))
                  {
                     if (v && !*v)
                        v = NULL;
                     if (v || value)
                        sql_sprintf(&s, "`%#S`=%#s,", name, v);
                  }
               } else
               {                // Correct
                  if (asprintf(&t, "cmnd/%s/%s", topic, name) < 0)
                     errx(1, "malloc");
                  sendfree(t, strlen(value), value);
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
                        const char *v2 = findval(j2);
                        if (!v2)
                           warnx("Not found %s in (%s)", name, topic);
                        else if (strcmp(v2, value))
                           warnx("Unable to set %s to %s on %s (is %s)", name, value, topic, v2);
                        else
                        {
                           if (info)
                              fprintf(stderr, "Updated %s to %s on %s (was %s)\n", name, value, topic, v ? : "unknown");
                           if (!strncmp(name, "Rule", 4) && isdigit(name[4]) && strcmp(value, "0"))
                           {    // Special case for Rule setting
                              if (asprintf(&t, "cmnd/%s/%s", topic, name) < 0)
                                 errx(1, "malloc");
                              sendfree(t, 1, "1");
                              if (getstat())
                                 warnx("Setting %s 1 did not work on %s", name, topic);
                           }
                        }
                     }
                     j_delete(&j2);
                  }
               }
            }
            j_delete(&j);
         }
      if (backup)
      {
         if (sql_back_s(&s) == ',')
         {
            sql_sprintf(&s, " WHERE `Topic`=%#s", sql_colz(res, "Topic"));
            sql_safe_query_s(&sql, &s);
         } else
            sql_free_s(&s);
      }
   }
   void configtopic(void) {
      SQL_RES *res = sql_safe_query_store_free(&sql, sql_printf("SELECT * FROM `%#S` WHERE `Topic`=%#s", sqltable, topic));
      if (!sql_fetch_row(res))
      {
         warnx("Not found in %s table: %s", sqltable, topic);
         sql_free_result(res);
         if (!backup)
            return;
         sql_safe_query_free(&sql, sql_printf("INSERT INTO `%#S` SET `Topic`=%#s", sqltable, topic));
         res = sql_safe_query_store_free(&sql, sql_printf("SELECT * FROM `%#S` WHERE `Topic`=%#s", sqltable, topic));
         if (!sql_fetch_row(res))
            errx(1, "create failed");
      }
      config(res);
      sql_free_result(res);
   }

   if (all)
   {
      sleep(2);                 // Get LWTs in...
      finding = 0;
      while (found)
      {
         struct found_s *f = found;
         found = f->next;
         topic = f->topic;
         configtopic();
         free(f->topic);
         free(f);
      }
   }
   {                            // Args
      while ((topic = poptGetArg(optCon)))
         configtopic();
   }
   // Tidy up
   mosquitto_loop_stop(mqtt, 1);
   free(foundtopic);
   free(foundpayload);
   mosquitto_destroy(mqtt);
   mosquitto_lib_cleanup();
   sql_close(&sql);
   poptFreeContext(optCon);
   return 0;
}
