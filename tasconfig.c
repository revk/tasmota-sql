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

#define       BACKLOG

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
   const char *base = NULL;
   int all = 0;
   int quiet = 0;
   int backup = 0;
   int fast = 0;

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
      { "base", 0, POPT_ARG_STRING, &base, 0, "Base settings", "topic" },
      { "quiet", 'q', POPT_ARG_NONE, &quiet, 0, "Don't show progress", NULL },
      { "backup", 'b', POPT_ARG_NONE, &backup, 0, "Backup device", NULL },
      { "fast", 'f', POPT_ARG_NONE, &fast, 0, "fast status check", NULL },
      { "all", 'a', POPT_ARG_NONE, &all, 0, "All devices", NULL },
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
   // As the response handling is all async, we use these as control variables.
   const char *topic = NULL;    // Current device topic
   char *fallbacktopic = NULL;
   int fields = 0;              // Number of fields
   int waiting = 0;             // Number of fields we are waiting for
   int changed = 0;             // Changed
   char **name = NULL;          // Set of fields names
   char **value = NULL;         // Set of (malloc'd) found values

   SQL sql;
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
      if (all)
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
   j_t status0 = NULL;
   struct found_s {
      struct found_s *next;
      char *topic;
   } *found = NULL;
   int finding = all;
   void message(struct mosquitto *mqtt, void *obj, const struct mosquitto_message *msg) {
      if (sqldebug)
         warnx("< %s %.*s", msg->topic, msg->payloadlen, (char *) msg->payload ? : "");
      obj = obj;
      // The Initial LWT are used to find a list of devices that are online.
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
         return;
      }
      if (!topic)
         return;                // Uh
      if (strncmp(s, "stat/", 5))
         return;
      char *t = s + 5;
      char *d = strchr(t, '/');
      if (!d || (strcmp(d, "/STATUS0") && strcmp(d, "/RESULT")) || d - t != strlen(topic) || strncmp(t, topic, d - t))
         return;
      j_t data = j_create();
      const char *je = j_read_mem(data, (char *) msg->payload, msg->payloadlen);
      if (je)
      {                         // Not JSON
         j_delete(&data);
         return;
      }
      if (!strcmp(d, "/STATUS0"))
      {
         j_delete(&status0);
         status0 = data;
         return;
      }
      if (!fields)
         return;
      // Process data
      void process(j_t j) {     // Process a value
         const char *tag = j_name(j);
         const char *val = j_val(j);
         if (!strcmp(tag, "NAME") && j_find(data, "GPIO"))
         {                      // template does not say template FFS
            tag = "Template";
            j = data;
         } else if (!strcmp(tag, "GPIO") && !strcmp(val, "Not supported"))
            return;
         int n = 0;
         for (n = 0; n < fields && strcasecmp(name[n], tag); n++);
         if (n >= fields)
         {
            warnx("Unexpected field %s in %s", tag, topic);
            return;
         }
         char *m = NULL;        // if malloced
         // Some special cases
         if (j_isobject(j) && (!strcasecmp(tag, "module") || !strcasecmp(tag, "sleep") || !strncasecmp(tag, "gpio", 4)))
            val = j_name(j_first(j));   // Crazy, field value is first tag
         else if (j_isobject(j) && !strncasecmp(tag, "rule", 4) && isdigit(tag[4]))
         {
            if (!strcmp(j_get(j, "State") ? : "", "OFF"))
               val = "0";
            else
               val = j_get(j, "Rules") ? : "0";
         } else if (j_isobject(j) && !strncasecmp(tag, "pulsetime", 9) && isdigit(tag[9]))
            val = j_get(j, "Set");
         else if (val && !strcasecmp(tag, "webserver"))
         {
            if (strstr(val, "Admin"))
               val = "2";
            else if (strstr(val, "User"))
               val = "1";
            else
               val = "0";
         } else if (val && !strncasecmp(tag, "setoption", 9) && isdigit(tag[9]))
         {
            if (!strcasecmp(val, "off"))
               val = "0";
            else if (!strcasecmp(val, "on"))
               val = "1";
         } else if (j_isobject(j) && (!strcasecmp(tag, "TimeStd") || !strcasecmp(tag, "TimeDst")))
         {                      // Comma separated list of numbers from object
            char *v = NULL;
            size_t l;
            FILE *f = open_memstream(&v, &l);
            for (j_t o = j_first(j); o; o = j_next(o))
               fprintf(f, ",%s", j_val(o));
            fclose(f);
            if (l)
               val = m = strdup(v + 1);
            free(v);
         } else if (j_isobject(j) && !strncasecmp(tag, "Timer", 5) && isdigit(tag[5]))
         {                      // Timer specifically
            if (!atoi(j_get(j, "Enable") ? : ""))
               val = NULL;
            else
               val = m = j_write_str(j);
         } else if (j_isobject(j))
         {                      // Treat as JSON as text
            val = m = j_write_str(j);
         }
         free(value[n]);
         if (!val || !*val)
            val = "0";
         value[n] = strdup(val);
         free(m);
      }
      j_t f = j_first(data);
      const char *tag = j_name(f);
      if (!strcasecmp(tag, "GroupTopic1"))
      {                         // List of values
         while (f)
         {
            process(f);
            f = j_next(f);
         }
      } else
         process(f);
      waiting--;
      j_delete(&data);
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

   void sendmqtt(char *topic, int len, const void *payload) {
      if (sqldebug)
         warnx("> %s %.*s", topic, len, (char *) payload ? : "");
      int e = mosquitto_publish(mqtt, NULL, topic, len, payload, 0, 0);
      if (e)
         errx(1, "MQTT publish failed %s", mosquitto_strerror(e));
   }

   void config(SQL_RES * res, int backup) {     // Configure a devices
      if (!base)
         base = strdup(sql_col(res, "_base") ? : "");
      SQL_RES *baseres = NULL;
      if (base)
      {
         baseres = sql_safe_query_store_free(&sql, sql_printf("SELECT * FROM `%#S` WHERE `Topic`=%#s", sqltable, base));
         if (!sql_fetch_row(baseres))
         {
            sql_free_result(baseres);
            baseres = NULL;
         }
      }
      // Check device is on line even...
      waiting = 0;
      fields = 0;
      value = malloc(sizeof(*value) * res->field_count);
      for (int n = 0; n < res->field_count; n++)
         value[n] = NULL;
      name = malloc(sizeof(*name) * res->field_count);
      for (int n = 0; n < res->field_count; n++)
         name[n] = res->fields[n].name;
      fields = res->field_count;

      if (sqldebug)
         warnx("Config for %s", topic);
      else if (!quiet)
         fprintf(stderr, "Checking %s\n", topic);

      char bl[1000];
      *bl = 0;
      char *t = NULL;
#ifdef	BACKLOG
      if (asprintf(&t, "cmnd/%s/Backlog0", fallbacktopic ? : topic) < 0)
         errx(1, "malloc");
#endif

      int catchup(void) {
         int bored = 1,
             last = -1;
         while (waiting)
         {
            if (last != waiting)
               bored = 10;
            else if (!--bored)
               break;
            last = waiting;
            usleep(100000);
         }
         return waiting;
      }
      const char *fetch(int n) {
         char *v = NULL;
         if (baseres && strcasecmp(name[n], "Topic"))
         {
            v = baseres->current_row[n];
            if (v && *v == '$' && v[1] == '{')
            {
               char *l = strdupa(v);
               l += 2;
               char *q = strchr(l, '}');
               if (q)
               {
                  *q++ = 0;
                  asprintf(&v, "%s%s", sql_colz(res, l), q);
               }
            }
         }
         if (!v)
            v = res->current_row[n];
         return v;
      }
      if(!fast)
      {
      int count = 0;
      for (int n = 0; n < fields; n++)
         if (*name[n] != '_' && (strncmp(name[n], "GroupTopic", 10) || name[n][10] == '1') && (strncmp(name[n], "GPIO", 4) || name[n][4] == '0') && (!setting || !strcasecmp(name[n], setting)) && (fetch(n) || backup))
         {
#ifdef	BACKLOG
            if (count >= 25 || strlen(bl) + strlen(name[n]) + 1 > sizeof(bl) - 1)
            {
               sendmqtt(t, strlen(bl), bl);
               catchup();
               *bl = 0;
               count = 0;
            }
            if (*bl)
               strcat(bl, ";");
            strcat(bl, name[n]);
#else
            if (waiting > 10)
               usleep(50000);
            asprintf(&t, "cmnd/%s/%s", fallbacktopic ? : topic, name[n]);
            sendmqtt(t, 0, NULL);
#endif
            waiting++;
            count++;
         }
#ifdef	BACKLOG
      if (*bl)
      {
         sendmqtt(t, strlen(bl), bl);
         catchup();
      }
      free(t);
#endif
      }
      sql_string_t s = { };
      sql_sprintf(&s, "UPDATE `%#S` SET ", sqltable);
      if (base && sql_colnum(res, "_base") >= 0 && strcmp(base, sql_colz(res, "_base")))
         sql_sprintf(&s, "`_base`=%#s,", base);
      if (status0)
      {
         const char *v;
         int n;
         const struct {
            const char *field;
            const char *where;
         } status[] = {
            { "_Version", "StatusFWR.Version" },        //
            { "_Hardware", "StatusFWR.Hardware" },      //
            { "_Booted", "StatusPRM.StartupUTC" },      //
            { "_Mac", "StatusNET.Mac" },        //
            { "_IPAddress", "StatusNET.IPAddress" },    //
         };
         for (int q = 0; q < sizeof(status) / sizeof(*status); q++)
         {
            if ((n = sql_colnum(res, status[q].field)) >= 0 && (v = j_get(status0, status[q].where)) && strcmp(v, res->current_row[n] ? : ""))
            {
               sql_sprintf(&s, "`%#S`=%#s,", status[q].field, v);
               if (!quiet)
                  fprintf(stderr, "Storing %s as %s on %s was %s\n", status[q].field, v, topic, res->current_row[n]);
            } else if (n < 0 && !quiet)
               warnx("No field %s", status[q].field);
         }
      }
      if (!fast)
      {
         if (backup)
         {                      // Reading from device
            for (int n = 0; n < fields; n++)
               if (value[n] && strcasecmp(name[n], "Topic") && strcmp(value[n] ? : "0", res->current_row[n] ? : "0"))
               {
                  sql_sprintf(&s, "`%#S`=%#s,", name[n], value[n]);
                  if (!quiet && fetch(n))
                  {
                     fprintf(stderr, "Storing %s as %s on %s\n", name[n], value[n], topic);
                     changed = 1;
                  }
               }
         } else
         {                      // Update device on changes
            const char *v;
            for (int n = 0; n < fields; n++)
            {
               v = fetch(n);
               if (value[n] && strcasecmp(name[n], "Topic") && strcmp(v ? : "0", res->current_row[n] ? : "0"))
                  sql_sprintf(&s, "`%#S`=%#s,", name[n], v);
               if (value[n] && strcmp(value[n] ? : "0", (v && *v) ? v : "0"))
               {
                  if (!v || !*v)
                     v = "0";
                  char *t;
                  if (asprintf(&t, "cmnd/%s/%s", fallbacktopic ? : topic, name[n]) < 0)
                     errx(1, "malloc");
                  waiting++;
                  sendmqtt(t, strlen(v), v);
                  if (!quiet)
                     fprintf(stderr, "Setting %s to %s on %s (was %s)\n", name[n], fetch(n), topic, value[n]);
                  if (!strncasecmp(name[n], "rule", 4) && isdigit(name[n][4]) && value[n] && strcmp(fetch(n), "0"))
                  {             // Special case for rules
                     fprintf(stderr, "Enabling %s\n", name[n]);
                     waiting++;
                     sendmqtt(t, 1, "1");       // Set RuleN 1
                  }
                  free(t);
               }
            }
            catchup();
            for (int n = 0; n < fields; n++)
               if (value[n] && strcmp(value[n] ? : "0", ((v = fetch(n)) && *v) ? v : "0"))
                  warnx("Failed to store %s as %s on %s (is %s)", name[n], fetch(n), topic, value[n]);
         }
         if (sqldebug)
            warnx("Missing data from %s (%d)", topic, waiting);
      }
      if (sql_back_s(&s) == ',')
      {
         sql_sprintf(&s, " WHERE `Topic`=%#s", sql_colz(res, "Topic"));
         sql_safe_query_s(&sql, &s);
      } else
         sql_free_s(&s);
      // Tidy
      fields = 0;
      for (int n = 0; n < res->field_count; n++)
         free(value[n]);
      free(value);
      free(name);
      if (baseres)
         sql_free_result(baseres);
   }

   void newdevice(void) {
      changed = 0;
      free(fallbacktopic);
      fallbacktopic = NULL;
      j_delete(&status0);
      char *t;
      if (asprintf(&t, "cmnd/%s/status0", topic) < 0)
         errx(1, "malloc");
      int retry = 3;
      while (!status0 && retry--)
      {
         sendmqtt(t, 0, NULL);
         int try = 30;
         while (try-- && !status0)
            usleep(100000);
         if (!status0)
            warnx("Failed to respond: %s", topic);
      }
      free(t);
      const char *v = NULL;
      if (status0 && (v = j_get(status0, "StatusMQT.MqttClient")) && asprintf(&fallbacktopic, "%s_fb", v) < 0)
         errx(1, "malloc");
   }

   void configtopic(int backup) {
      SQL_RES *res = sql_safe_query_store_free(&sql, sql_printf("SELECT * FROM `%#S` WHERE `Topic`=%#s", sqltable, topic));
      if (!sql_fetch_row(res))
      {
         int hasbase = sql_colnum(res, "_base");
         sql_free_result(res);
         if (!backup)
         {
            warnx("Not found in database: %s", topic);
            return;
         }
         warnx("Creating new device in database: %s", topic);
         sql_string_t s = { };
         sql_sprintf(&s, "INSERT INTO `%#S` SET `Topic`=%#s", sqltable, topic);
         if (base && hasbase >= 0)
            sql_sprintf(&s, ",`_base`=%#s", base);
         sql_safe_query_s(&sql, &s);
         res = sql_safe_query_store_free(&sql, sql_printf("SELECT * FROM `%#S` WHERE `Topic`=%#s", sqltable, topic));
         if (!sql_fetch_row(res))
            errx(1, "create failed");
      }
      config(res, backup);
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
         newdevice();
         configtopic(backup);
         if (waiting)
         {
            sleep(1);
            configtopic(backup);
         }
         if (!waiting && backup && base && changed)
            configtopic(0);
         free(f->topic);
         free(f);
      }
   }
   {                            // Args
      while ((topic = poptGetArg(optCon)))
      {
         newdevice();
         configtopic(backup);
         if (waiting)
         {
            sleep(1);
            configtopic(backup);
         }
         if (!waiting && backup && base && changed)
            configtopic(0);
      }
   }
   // Tidy up
   mosquitto_loop_stop(mqtt, 1);
   mosquitto_destroy(mqtt);
   mosquitto_lib_cleanup();
   sql_close(&sql);
   poptFreeContext(optCon);
   return 0;
}
