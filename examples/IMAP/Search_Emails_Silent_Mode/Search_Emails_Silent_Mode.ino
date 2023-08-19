/**
 * This example shows how to set messages flags.
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2023 mobizt
 *
 */

// This example shows how to search all messages with the keywords in the opened mailbox folder.

/**
 * To use library in silent mode (no debug printing and callback), please define this macro in src/ESP_Mail_FS.h.
 * #define SILENT_MODE
 */

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

#include <ESP_Mail_Client.h>

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

#define IMAP_HOST "<host>"

#define IMAP_PORT esp_mail_imap_port_143

#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

void printImapData(IMAP_Status status);

void printAllMailboxesInfo(IMAPSession &imap);

void printSelectedMailboxInfo(SelectedFolderInfo sFolder);

void printMessages(MB_VECTOR<IMAP_MSG_Item> &msgItems, bool headerOnly);

void printAttacements(MB_VECTOR<IMAP_Attach_Item> &atts);

IMAPSession imap;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
    while (!Serial)
        ;
#endif

    Serial.println();

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    unsigned long ms = millis();
#endif

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
#endif
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    MailClient.networkReconnect(true);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    MailClient.clearAP();
    MailClient.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    Session_Config config;

    config.server.host_name = IMAP_HOST;
    config.server.port = IMAP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;

    IMAP_Data imap_data;

    imap_data.fetch.uid.clear();

    imap_data.search.criteria = F("SEARCH RECENT");

    imap_data.search.unseen_msg = true;

    imap_data.storage.saved_path = F("/email_data");

    imap_data.storage.type = esp_mail_file_storage_type_flash;

    imap_data.download.header = true;
    imap_data.download.text = true;
    imap_data.download.html = true;
    imap_data.download.attachment = true;
    imap_data.download.inlineImg = true;

    imap_data.enable.html = true;
    imap_data.enable.text = true;

    imap_data.enable.recent_sort = true;

    imap_data.enable.download_status = true;

    imap_data.limit.search = 5;

    imap_data.limit.msg_size = 512;

    imap_data.limit.attachment_size = 1024 * 1024 * 5;

    imap.setTCPTimeout(10);

    if (!imap.connect(&config, &imap_data))
    {
        MailClient.printf("Connection error, Error Code: %d, Reason: %s", imap.errorCode(), imap.errorReason().c_str());
        return;
    }

    if (imap.isAuthenticated())
        Serial.println("\nSuccessfully logged in.");
    else
        Serial.println("\nConnected with no Auth.");

    printAllMailboxesInfo(imap);

    if (!imap.selectFolder(F("INBOX")))
    {
        MailClient.printf("Folder selection error, Error Code: %d, Reason: %s", imap.errorCode(), imap.errorReason().c_str());
        return;
    }

    printSelectedMailboxInfo(imap.selectedFolder());

    if (MailClient.readMail(&imap, false))
    {
        printImapData(imap.status());
    }
    else
    {
        MailClient.printf("Message searching error, Error Code: %d, Reason: %s", imap.errorCode(), imap.errorReason().c_str());
    }

    imap.closeSession();

    imap.empty();
}

void loop()
{
}

void printImapData(IMAP_Status status)
{

    if (status.success())
    {
        MailClient.printf("\nFound %d messages\n\n", imap.selectedFolder().searchCount());

        IMAP_MSG_List msgList = imap.data();
        printMessages(msgList.msgItems, imap.headerOnly());

        imap.empty();
    }
}

void printAllMailboxesInfo(IMAPSession &imap)
{
    FoldersCollection folders;

    if (imap.getFolders(folders))
    {
        for (size_t i = 0; i < folders.size(); i++)
        {
            FolderInfo folderInfo = folders.info(i);
            MailClient.printf("%s%s%s", i == 0 ? "\nAvailable folders: " : ", ", folderInfo.name, i == folders.size() - 1 ? "\n" : "");
        }
    }
}

void printSelectedMailboxInfo(SelectedFolderInfo sFolder)
{
    /* Show the mailbox info */
    MailClient.printf("\nInfo of the selected folder\nTotal Messages: %d\n", sFolder.msgCount());
    MailClient.printf("UID Validity: %d\n", sFolder.uidValidity());
    MailClient.printf("Predicted next UID: %d\n", sFolder.nextUID());
    if (sFolder.unseenIndex() > 0)
        MailClient.printf("First Unseen Message Number: %d\n", sFolder.unseenIndex());
    else
        MailClient.printf("Unseen Messages: No\n");

    if (sFolder.modSeqSupported())
        MailClient.printf("Highest Modification Sequence: %llu\n", sFolder.highestModSeq());
    for (size_t i = 0; i < sFolder.flagCount(); i++)
        MailClient.printf("%s%s%s", i == 0 ? "Flags: " : ", ", sFolder.flag(i).c_str(), i == sFolder.flagCount() - 1 ? "\n" : "");

    if (sFolder.flagCount(true))
    {
        for (size_t i = 0; i < sFolder.flagCount(true); i++)
            MailClient.printf("%s%s%s", i == 0 ? "Permanent Flags: " : ", ", sFolder.flag(i, true).c_str(), i == sFolder.flagCount(true) - 1 ? "\n" : "");
    }
}

void printAttacements(MB_VECTOR<IMAP_Attach_Item> &atts)
{
    MailClient.printf("Attachment: %d file(s)\n****************************\n", atts.size());
    for (size_t j = 0; j < atts.size(); j++)
    {
        IMAP_Attach_Item att = atts[j];
        MailClient.printf("%d. Filename: %s, Name: %s, Size: %d, MIME: %s, Type: %s, Description: %s, Creation Date: %s\n", j + 1, att.filename, att.name, att.size, att.mime, att.type == esp_mail_att_type_attachment ? "attachment" : "inline", att.description, att.creationDate);
    }
    Serial.println();
}

void printMessages(MB_VECTOR<IMAP_MSG_Item> &msgItems, bool headerOnly)
{

    for (size_t i = 0; i < msgItems.size(); i++)
    {
        IMAP_MSG_Item msg = msgItems[i];

        Serial.println("****************************");

        MailClient.printf("Number: %d\n", msg.msgNo);
        MailClient.printf("UID: %d\n", msg.UID);

        // The attachment status in search may be true in case the "multipart/mixed"
        // content type header was set with no real attachtment included.
        MailClient.printf("Attachment: %s\n", msg.hasAttachment ? "yes" : "no");

        MailClient.printf("Messsage-ID: %s\n", msg.ID);

        if (strlen(msg.flags))
            MailClient.printf("Flags: %s\n", msg.flags);
        if (strlen(msg.acceptLang))
            MailClient.printf("Accept Language: %s\n", msg.acceptLang);
        if (strlen(msg.contentLang))
            MailClient.printf("Content Language: %s\n", msg.contentLang);
        if (strlen(msg.from))
            MailClient.printf("From: %s\n", msg.from);
        if (strlen(msg.sender))
            MailClient.printf("Sender: %s\n", msg.sender);
        if (strlen(msg.to))
            MailClient.printf("To: %s\n", msg.to);
        if (strlen(msg.cc))
            MailClient.printf("CC: %s\n", msg.cc);
        if (strlen(msg.date))
        {
            MailClient.printf("Date: %s\n", msg.date);
            MailClient.printf("Timestamp: %d\n", (int)MailClient.Time.getTimestamp(msg.date));
        }
        if (strlen(msg.subject))
            MailClient.printf("Subject: %s\n", msg.subject);
        if (strlen(msg.reply_to))
            MailClient.printf("Reply-To: %s\n", msg.reply_to);
        if (strlen(msg.return_path))
            MailClient.printf("Return-Path: %s\n", msg.return_path);
        if (strlen(msg.in_reply_to))
            MailClient.printf("In-Reply-To: %s\n", msg.in_reply_to);
        if (strlen(msg.references))
            MailClient.printf("References: %s\n", msg.references);
        if (strlen(msg.comments))
            MailClient.printf("Comments: %s\n", msg.comments);
        if (strlen(msg.keywords))
            MailClient.printf("Keywords: %s\n", msg.keywords);

        if (!headerOnly)
        {
            if (strlen(msg.text.content))
                MailClient.printf("Text Message: %s\n", msg.text.content);
            if (strlen(msg.text.charSet))
                MailClient.printf("Text Message Charset: %s\n", msg.text.charSet);
            if (strlen(msg.text.transfer_encoding))
                MailClient.printf("Text Message Transfer Encoding: %s\n", msg.text.transfer_encoding);
            if (strlen(msg.html.content))
                MailClient.printf("HTML Message: %s\n", msg.html.content);
            if (strlen(msg.html.charSet))
                MailClient.printf("HTML Message Charset: %s\n", msg.html.charSet);
            if (strlen(msg.html.transfer_encoding))
                MailClient.printf("HTML Message Transfer Encoding: %s\n\n", msg.html.transfer_encoding);

            if (msg.rfc822.size() > 0)
            {
                MailClient.printf("\r\nRFC822 Messages: %d message(s)\n****************************\n", msg.rfc822.size());
                printMessages(msg.rfc822, headerOnly);
            }

            if (msg.attachments.size() > 0)
                printAttacements(msg.attachments);
        }

        Serial.println();
    }
}
