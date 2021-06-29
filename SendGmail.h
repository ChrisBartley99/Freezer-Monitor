
/**
 * This example will send the Email in plain text version.
 * 
 * Created by K. Suwatchai (Mobizt)
 * 
 * Email: suwatchai@outlook.com
 * 
 * Github: https://github.com/mobizt/ESP-Mail-Client
 * 
 * Copyright (c) 2020 mobizt
 *
*/


#include <ESP_Mail_Client.h>

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);



void SendGmail(char *Dest, char *Subj, char *Msg)
{  
  /** Dis-able the debug via Serial port
  */
  smtp.debug(0);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTPSERVER;
  session.server.port = SMTP_PORT;
  session.login.email = EMAIL_SENDER_ACCOUNT;
  session.login.password = EMAIL_SENDER_PASSWORD;
  session.login.user_domain = "mydomain.net";

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP Mail";
  message.sender.email = EMAIL_SENDER_ACCOUNT;

  /* the actual email content */
  message.subject = Subj ;
  message.addRecipient("ToWhomItMayConcern", Dest);
  message.text.content = Msg;

  
//  message.text.charSet = "us-ascii";
//  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
//  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
//  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Set the custom message header */
//  message.addHeader("Message-ID: <abcde.fghij@gmail.com>");

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}



/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    Serial.println("----------------");
    Serial.printf("Message sent success: %d\n", status.completedCount());
    Serial.printf("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      localtime_r(&result.timesstamp, &dt);

      Serial.printf("Message No: %d\n", i + 1);
      Serial.printf("Status: %s\n", result.completed ? "success" : "failed");
      Serial.printf("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      Serial.printf("Recipient: %s\n", result.recipients);
      Serial.printf("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}