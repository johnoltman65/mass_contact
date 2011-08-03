TABLE OF CONTENTS:
 * Introduction
 * Features
 * Installation
 * Configuration and Setup
 * More Information
 * Changes from Drupal 6 to Drupal 7
 * Troubleshooting and Miscellaneous


INTRODUCTION:
The Mass Contact module started out as a modified version of the core Contact
module. It allows anyone with permission to send a message to users, defined
by the role or group of roles (or even to all registered users) they belong
to.


FEATURES:
Here are a list of some of the features this module has:
 * You can send a message to one or more groups (referred to as categories) of
   users, which are specified by role.
 * Large recipient lists may be broken up into smaller chunks.
 * The site administrator can control how many messages are allowed to be sent
   by a single person in an hour.
 * The message may be sent such that the recipients' e-mail addresses are
   hidden from each other, via the BCC (blind carbon copy) field.
 * The message may be sent as plain text or HTML, even specifying the input
   format filter to use.
 * The message may include one or more binary file attachments.
 * The site administrator may specify different texts to be placed at the
   beginning and/or the end of every message that is sent out.
 * A copy of the message may be saved as a node.
 * Users may opt-out, by category, of receiving mass mailings on their account
   settings page.


INSTALLATION:
This module is installed in a standard way. Generic instructions for how to do
that can be found here: http://drupal.org/getting-started/install-contrib


CONFIGURATION AND SETUP:
The place where you create categories and modify the module's settings is
found in the same place as core's Contact module, the Site building section of
the Administer page (admin/config/system/mass_contact).

You need to add at least one category before sending any mass e-mails, which
can be done at the same location where the administrative settings are.


MORE INFORMATION:
This module works by sending a single e-mail to your mail server with the
recipients' e-mail addresses in either the 'To:' or 'Bcc:' field. The mail
server is then responsible for parsing out the recipients' addresses and
forwarding the message along to everyone.

Here is some scaling information:
 * This module retrieves user ids and emails in a scaled way: no
 * This module sends email in a scaled way: yes, within server limits
 * This module keeps connections up while the long process continues: no

Here are all the menu items/links that are available and what they do:
URL               | Label             | Description        | To have access to
                  |                   |                    | this URL, users
                  |                   |                    | must have this
                  |                   |                    | permission
------------------------------------------------------------------------------
/admin/config/    | Mass Contact      | The main           | administer mass
system/           |                   | administrative     | contact
mass_contact      |                   | interface, which   |
                  |                   | defaults to the    |
                  |                   | Category list page |
                  |                   | below.             |
------------------------------------------------------------------------------
/admin/config/    | Category list     | List the currently | administer mass
system/           |                   | defined            | contact
mass_contact/list |                   | categories.        |
------------------------------------------------------------------------------
/admin/config/    | Add category      | Add a new          | administer mass
system/           |                   | category.          | contact
mass_contact/add  |                   |                    |
------------------------------------------------------------------------------
/admin/config/    | Edit Mass Contact | Edit an existing   | administer mass
system/           | category (the     | category.          | contact
mass_contact/     | 'edit' operation  |                    |
edit/$category_id | in the Category   |                    |
                  | list)             |                    |
------------------------------------------------------------------------------
/admin/config/    | Delete Mass       | Delete an existing | administer mass
system/           | Contact category  | category.          | contact
mass_contact/     | (the 'delete'     |                    |
delete/           | operation in the  |                    |
$category_id      | Category list)    |                    |
------------------------------------------------------------------------------
/admin/config/    | Settings          | Administrative     | administer mass
system/           |                   | settings to modify | contact
mass_contact/     |                   | how Mass Contact   |
settings          |                   | operates. There    |
                  |                   | are three sub      |
                  |                   | pages under this   |
                  |                   | one.               |
------------------------------------------------------------------------------
/mass_contact     | Mass Contact      | The main Mass      | send mass
                  |                   | Contact form for   | contact e-mails
                  |                   | sending messages.  |
------------------------------------------------------------------------------
/node/add/        | Mass Contact      | The form for       | create
mass_contact      |                   | adding a Mass      | mass_contact
                  |                   | Contact content    | content
                  |                   | item. This is not  |
                  |                   | really useful on   |
                  |                   | its own.           |

Although it uses the old paths, an easier to view table can be found here:
http://drupal.org/node/760548#comment-2912412


CHANGES FROM DRUPAL 6 TO DRUPAL 7:
Over time, this module has evolved to something more than just a basic contact
module to multiple users. Due to that fact, the Drupal 7 version diverges from
what the module originally did to what people expect of a contributed module.
As an example, I've moved the module's configuration settings to where most
people expect to find them: the Configuration page. I also now have the main
menu item enabled by default.

I'm changing the way I develop modules that interact with other modules. In
the past, I chose to do everything myself, rather than depend on other modules
for bits of functionality. Instead, I'm going to try creating more modular
code and playing nice with other modules. This may mean more dependencies, and
dealing with other people's broken code (rather than just my own), but I'm
hoping this will ultimately lead to a better module.

With that in mind, and due to the fact that Drupal 7 now assumes everything is
an HTML e-mail and converts it to plain text by default,  I'm using the Mime
Mail module (http://drupal.org/project/mimemail) to send HTML e-mail, and
e-mail with attachments, rather than do that part myself. I'll still handle
the basic plain text e-mails.


TROUBLESHOOTING AND MISCELLANEOUS:
 * If you are using the SMTP Authentication Support or PHPMailer modules to
   send e-mail from your Drupal installation and are experiencing problems, be
   sure to check the issue queues for those modules for solutions, as well.
 * There is something the site administrator and/or the sender needs to keep
   in mind when breaking up a large recipient list into smaller chunks and
   sending the message as BCC (hiding the recipients from each other), and
   that is that the sender will receive a copy of the message for every group
   of recipients the list is broken up into. That is normal behavior and
   cannot be changed.
 * If your category permissions are not showing up correctly, check your
   category name and make sure you don't have any stray characters or any
   characters that Drupal doesn't allow.
 * If you experience "return-path" errors when sending e-mail, you can try the
   Return-Path module (http://drupal.org/project/returnpath) to see if that
   solves your problem.
