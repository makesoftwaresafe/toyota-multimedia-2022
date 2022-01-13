/*
 * P7V - Verify PKCS#7 Signed Packages
 * Copyright (C) 2012 Cedric Hombourger <chombourger@gmail.com>
 * License: GNU GPL (GNU General Public License, see COPYING-GPL)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <nspr.h>

#include <cert.h>
#include <nss.h>
#include <cms.h>

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
     
#include "internal.h"

/** Usage to be provided during the verification process. */
#define NSS_CERT_USAGE certUsageObjectSigner

/** Size of the arena to be allocated. */
#define NSS_ARENA_SIZE 4096

/** Flag set by --verbose. */
static int verbose;

/** Flag set by --no-verify. */
static int no_verify;

/** Path to the NSS database. */
static char *nssdir = 0;

/* P7V error codes. */
typedef enum {
   P7V_OK                        = 0, /**< Success. */
   P7V_INPUT_OPEN_FAILED         = 1, /**< Failed to open input file. */
   P7V_OUTPUT_OPEN_FAILED        = 2, /**< Failed to open output file. */
   P7V_OUTPUT_WRITE_FAILED       = 3, /**< Failed to write to output file. */
   P7V_VERIFICATION_FAILED       = 4, /**< Package verification failed! */
   P7V_NSS_INIT_FAILED           = 5, /**< NSS Library initialization failed. */
   P7V_CMS_DECODER_START_FAILED  = 6, /**< Failed to initiate decoding. */
   P7V_CMS_DECODER_UPDATE_FAILED = 7, /**< Failed to feed decoder. */
   P7V_CMS_DECODER_FINISH_FAILED = 8, /**< Failure while closing decoder. */
   P7V_CMS_CERT_IMPORT_FAILED    = 9, /**< Failed to import certificate(s). */
} p7v_result_t;

/** The package decoder structure. Holds variables needed to work with NSS. */
typedef struct {
   /** Memory pool for NSS. */
   PLArenaPool *p_arena;
   /** Output file stream. */
   FILE *output;
   /** Flag set if a write error occurs while decoding. */
   int write_error;
   /** The NSS Decoder context. */
   NSSCMSDecoderContext *p_context;
} p7v_decoder_t;

/**
  * Initialize P7V.
  *
  * @param dir NSS database and configuration directory.
  *
  * @return P7V_OK on success, an error code otherwise.
  *
  */
static p7v_result_t
init (const char *dir) {

   p7v_result_t result;

   trace_init ();
   TRACE3 (("called with dir='%s'", dir));

   PR_Init (PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);
   SECStatus sts = NSS_InitReadWrite (dir);
   TRACE4 (("NSS_Init() returned %d", sts));

   if (sts == SECSuccess) {
      TRACE4 (("NSS initialized"));
      result = P7V_OK;
   }
   else {
      TRACE1 (("Failed to initialize NSS (%d)", sts));
      result = P7V_NSS_INIT_FAILED;
   }

   TRACE3 (("exiting"));
   return result;
}

/**
  * Callback function called by the decoder whenever decoded data
  * becomes available. This callback function is used to write the
  * decoded stream to the user-defined output.
  *
  * @param arg opaque pointer to the decoder descriptor.
  * @param buf pointer to the decoded data
  * @param len length of the decoded data
  *
  */
static void
content_cb (void *arg, const char *buf, unsigned long len) {

   p7v_decoder_t *p_decoder = arg;
   size_t written;

   TRACE3 (("called with arg=%p, buf=%p, len=%lu", arg, buf, len));

   /* Write decoded output. */
   written = fwrite (buf, len, 1, p_decoder->output);
   if (written != 1) {
      TRACE1 (("failed to write %u bytes!", len));
      p_decoder->write_error = 1;
   }

   TRACE3 (("exiting"));
}

/**
  * Setup the p7v decoder.
  *
  * @param output pointer to the output stream.
  * @param p_decoder pointer to the decoder descriptor
  *
  * @return P7V_OK on success, an error code otherwise.
  *
  */
static p7v_result_t
decoder_setup (FILE *output, p7v_decoder_t *p_decoder) {

   p7v_result_t result;

   TRACE3 (("called with output=%p, p_decoder=%p", output, p_decoder));

   /* Create pool arena. */
   p_decoder->p_arena = PORT_NewArena (NSS_ARENA_SIZE);
   if (p_decoder->p_arena != 0) {

      NSSCMSDecoderContext *ctx = NSS_CMSDecoder_Start (
         p_decoder->p_arena, content_cb, p_decoder, 0, 0, 0, 0
      );
      TRACE4 (("NSS_CMSDecoder_Start() returned %p", ctx));

      if (ctx == 0) {
         TRACE1 (("Failed to initialize CMS decoder!"));
         result = P7V_CMS_DECODER_START_FAILED;
      }
      else {
         p_decoder->output = output;
         p_decoder->p_context = ctx;
         p_decoder->write_error = 0;
         result = P7V_OK;
      }
   }

   TRACE3 (("exiting with result=%d", result));
   return result;
}

/**
  * Feed input data into the decoder.
  *
  * @param decoder the p7v decoder descriptor.
  * @param buf pointer to the input data to be injected.
  * @param len length of the data provided.
  *
  * @return P7V_OK on success, an error code otherwise.
  *
  */
static p7v_result_t
decoder_feed (p7v_decoder_t decoder, const char *buf, unsigned long len) {

   p7v_result_t result;

   TRACE3 (("called with decoder=%p, buf=%p, len=%p", decoder, buf, len));

   SECStatus sts = NSS_CMSDecoder_Update (decoder.p_context, buf, len);
   TRACE4 (("NSS_CMSDecoder_Update() returned %d", sts));

   if (sts == SECSuccess) {
      result = P7V_OK;
   }
   else {
      TRACE1 (("Failed to feed CMS decoder (%d)!", sts));
      result = P7V_CMS_DECODER_UPDATE_FAILED;
   }

   TRACE3 (("exiting with result=%d", result));
   return result;
}

/**
  * Verify the nth signature of the signed message.
  *
  * @param decoder the decoder descriptor.
  * @param p_signed_data pointer to the signed data descriptor.
  * @param signer index of the signer to be checked.
  *
  * @return P7V_OK on success, an error code otherwise.
  *
  */
static p7v_result_t
verify_signer (p7v_decoder_t decoder, NSSCMSSignedData *p_signed_data, unsigned int signer) {

   p7v_result_t result;
   NSSCMSSignerInfo *p_signer_info;
   NSSCMSVerificationStatus verif_status;
   const char *p_verif_status_string;
   SECCertificateUsage usage;
   CERTCertificate *p_issuer_cert;
   SECStatus sts;
   int flags;

   TRACE3 (("called with p_signed_data=%p, signer=%u", p_signed_data, signer));

   /* Get nth signer info. */
   p_signer_info = NSS_CMSSignedData_GetSignerInfo (p_signed_data, signer);
   TRACE4 (("NSS_CMSSignedData_GetSignerInfo() returned %p", p_signer_info));

   /* Get verification status. */

   sts = NSS_CMSSignedData_VerifySignerInfo (
      p_signed_data, signer, CERT_GetDefaultCertDB (), NSS_CERT_USAGE
   );
   TRACE4 (("NSS_CMSSignedData_VerifySignerInfo() returned %d", sts));

   verif_status = NSS_CMSSignerInfo_GetVerificationStatus (p_signer_info);
   TRACE4 (("NSS_CMSSignerInfo_GetVerificationStatus() returned %d", verif_status));

   p_verif_status_string = NSS_CMSUtil_VerificationStatusToString (verif_status);
   TRACE4 (("NSS_CMSUtil_VerificationStatusToString() returned '%s'", p_verif_status_string));

   /* Print signer if verbose. */

   if (verbose != 0) {
      char *p_signer_cn;
      static char empty[] = { "" };

      p_signer_cn = NSS_CMSSignerInfo_GetSignerCommonName (p_signer_info);
      if (p_signer_cn == NULL) p_signer_cn = empty;
      fprintf (stderr, "signed by '%s'\n", p_signer_cn);
      if (p_signer_cn != empty) {
         PORT_Free (p_signer_cn);
      }
   }

   if (sts != SECSuccess) {
      TRACE1 (("Signer verification failed (%d)!", sts));
      result = P7V_VERIFICATION_FAILED;
   }
   else {
      result = P7V_OK;
   }

   TRACE3 (("exiting with result=%d", result));
   return result; 
}

/**
  * Verify the signatures found in the signed message.
  *
  * @param decoder the decoder descriptor
  * @param p_signed_data pointer to the signed data descriptor.
  *
  * @return P7V_OK on success, an error code otherwise.
  *
  */
static p7v_result_t
verify_signers (p7v_decoder_t decoder, NSSCMSSignedData *p_signed_data) {

   p7v_result_t result = P7V_OK;
   unsigned int signers_count;
   unsigned int signer;
   SECStatus sts;

   TRACE3 (("called with p_signed_data=%p", p_signed_data));

   /* Import certificates found in the signed message. */
   sts = NSS_CMSSignedData_ImportCerts (
      p_signed_data, CERT_GetDefaultCertDB (), NSS_CERT_USAGE, PR_TRUE
   );
   TRACE4 (("NSS_CMSSignedData_ImportCerts() returned %d", sts));

   if (sts != SECSuccess) {
      TRACE1 (("Import of certificates failed (%d)", sts));
      result = P7V_CMS_CERT_IMPORT_FAILED;
   }
   else {
      /* Get number of signers for this message and loop on each to verify them
       * individually. */
      signers_count = NSS_CMSSignedData_SignerInfoCount (p_signed_data);
      TRACE4 (("NSS_CMSSignedData_SignerInfoCount() returned %u", signers_count));

      for (signer = 0; signer < signers_count; signer ++) {
         result = verify_signer (decoder, p_signed_data, signer);
         if (result != P7V_OK) break;
      }
   }

   TRACE3 (("exiting with result=%d", result));
   return result;
}

/**
  * Check the message integrity and in particular certificates of the signer(s).
  *
  * @param decoder the decoder descritor
  * @param p_message pointer to the message to be checked.
  *
  * @return P7V_OK on success, an error code otherwise.
  *
  */
static p7v_result_t
check_message (p7v_decoder_t decoder, NSSCMSMessage *p_message) {

   NSSCMSSignedData *p_signed_data;
   unsigned int level_count, i;
   p7v_result_t result = P7V_OK;

   TRACE3 (("called with decoder=%p, p_message=%p", decoder, p_message));

   if (NSS_CMSMessage_IsSigned (p_message)) {

      level_count = NSS_CMSMessage_ContentLevelCount (p_message);
      TRACE4 (("NSS_CMSMessage_ContentLevelCount() returned %u", level_count));

      for (i = 0; i < level_count; i++) {
         NSSCMSContentInfo *p_content_info;
         SECOidTag type_tag;

         p_content_info = NSS_CMSMessage_ContentLevel (p_message, i);
         TRACE4 (("NSS_CMSMessage_ContentLevel() returned %p", p_content_info));

         type_tag = NSS_CMSContentInfo_GetContentTypeTag (p_content_info);
         TRACE4 (("NSS_CMSContentInfo_GetContentTypeTag() returned %u", type_tag));

         if (type_tag == SEC_OID_PKCS7_SIGNED_DATA) {
            p_signed_data = (NSSCMSSignedData *)
               NSS_CMSContentInfo_GetContent (p_content_info);
            TRACE4 (("NSS_CMSContentInfo_GetContent() returned %p", p_signed_data));

            result = verify_signers (decoder, p_signed_data);
            if (result != P7V_OK) break;
         }
      }
   }

   TRACE3 (("exiting with result=%d", result));
   return result; 
}

/**
  * Finish the decoding process and verify signed content.
  *
  * @param decoder the decoder descriptor.
  * @param skip_verification non-zero to skip verification
  *
  * @return P7V_OK on success, an error code otherwise.
  *
  */
static p7v_result_t
decoder_finish (p7v_decoder_t decoder, int skip_verification) {

   p7v_result_t result = P7V_OK;
   NSSCMSMessage *p_message;

   TRACE3 ((
      "called with decoder=%p, skip_verification=%d",
      decoder, skip_verification
   ));

   p_message = NSS_CMSDecoder_Finish (decoder.p_context);
   TRACE4 (("NSS_CMSDecoder_Finish() returned %p", p_message));

   if (p_message != 0) {

      /* Verify the signature(s). */
      if (skip_verification == 0) {
         result = check_message (decoder, p_message);
      }
      else if (verbose != 0) {
         fprintf (
            stderr, PACKAGE_NAME ": warning: "
            "verification skipped as requested!\n"
         );
      }

      /* Free memory used by the CMS message. */
      NSS_CMSMessage_Destroy (p_message);
      p_message = 0;
   }
   else {
      TRACE1 (("Decoder eventually failed (%d)!", PORT_GetError ()));
      result = P7V_CMS_DECODER_FINISH_FAILED;
   }

   if (decoder.p_arena != 0) {
      PORT_FreeArena (decoder.p_arena, PR_FALSE);
      decoder.p_arena = 0;
   }

   (void) NSS_Shutdown ();

   TRACE3 (("exiting with result=%d", result));
   return result;
}

int
main (int argc, char **argv) {

   FILE *fin, *fout;
   char *output = 0;
   p7v_decoder_t decoder;
   int result;
   int c;
     
   while (1) {
      static struct option long_options [] = {
         { "no-verify", no_argument, &no_verify, 1 },
         { "verbose",   no_argument, &verbose,   1 },
         { 0,           0,           0,          0 }
      };

      /* getopt_long stores the option index here. */
      int option_index = 0;
     
      c = getopt_long (argc, argv, "d:o:nv", long_options, &option_index);
     
      /* Detect the end of the options. */
      if (c == -1) {
         break;
      }
    
      switch (c) {

         case 'd':
            nssdir = optarg;
            break;

         case 'n':
            no_verify = 1;
            break;

         case 'o':
            output = optarg;
            break;

         case 'v':
            verbose = 1;
            break;
      }
   }

   /* Print package name and version if verbose. */
   if (verbose != 0) {
      fprintf (stderr, PACKAGE_NAME " version " PACKAGE_VERSION "\n");
   }

   /* Open input file for reading. Use stdin if none supplied. */
   if (optind < argc) {
      fin = fopen (argv [optind], "r");
      if (fin == 0) {
         fprintf (stderr, "failed to open input file '%s'!\n", argv [optind]);
         return P7V_INPUT_OPEN_FAILED;
      }
      else if (verbose != 0) {
         fprintf (stderr, "opened '%s' for reading.\n", argv [optind]);
      }
   }
   else {
      fin = stdin;
   }

   /* Open output file for writing. Use stdout if none supplied. */
   if (output != 0) {
      fout = fopen (output, "w");
      if (fout == 0) {
         fprintf (stderr, "failed to open output file '%s'!\n", output);
         return P7V_OUTPUT_OPEN_FAILED;
      }
      else if (verbose != 0) {
         fprintf (stderr, "opened '%s' for writing.\n", output);
      }
   }
   else {
      fout = stdout;
   }

   /* Initialize ourselves. */
   result = (int) init (nssdir);
   if (result != P7V_OK) {
      fprintf (stderr, "initialization failed (%d)!\n", result);
      return result;
   }
   else if (verbose != 0) {
      fprintf (stderr, "initialized.\n");
   }

   /* Setup the decoder for the PCKS#7 package. */
   result = decoder_setup (fout, &decoder);
   if (result != P7V_OK) {
      fprintf (stderr, "failed to setup decoder (%d)\n", result);
      return result;
   }

   /* Read the input data and pass it through the decoder. */
   while (!feof (fin)) {
      static char buffer [4096];
      int r = fread (buffer, 1, sizeof (buffer), fin);
      if (r > 0) {
         result = decoder_feed (decoder, buffer, r); 
         if (result != P7V_OK) break;
         if (decoder.write_error != 0) {
            TRACE1 (("aborting decoding due to a write error!"));
            result = P7V_OUTPUT_WRITE_FAILED;
            break;
         }
      }
   }

   /* Close input and output streams. */
   if (fout != stdout) fclose (fout);
   if (fin != stdin) fclose (fin);

   /* Have the decoder verify the output and clean-up things. */
   if (result == P7V_OK) {
      result = decoder_finish (decoder, no_verify);
   }

   /* On failure, the output shall be deleted (if not stdout). */
   if (result != P7V_OK) {
      if (fout != stdout) {
         (void) unlink (output);
      }
   }
   else if (verbose != 0) {
      fprintf (stderr, "data extracted.\n");
   }

   if (verbose != 0) {
      fprintf (stderr, "exiting with status %d\n", result);
   }

   return result;
}

