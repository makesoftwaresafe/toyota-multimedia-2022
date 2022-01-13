/*
 * Copyright (C) 2008 OpenedHand Ltd.
 * Copyright (C) 2012 Jens Georg.
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 *         Jens Georg <mail@jensge.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

namespace GUPnP {
        public class DIDLLiteWriter : GLib.Object {
                [CCode (cname = "GUPNP_DIDL_LITE_WRITER_NAMESPACE_DC")]
                public const string NAMESPACE_DC;

                [CCode (cname = "GUPNP_DIDL_LITE_WRITER_NAMESPACE_UPNP")]
                public const string NAMESPACE_UPNP;

                [CCode (cname = "GUPNP_DIDL_LITE_WRITER_NAMESPACE_DLNA")]
                public const string NAMESPACE_DLNA;
        }

        public errordomain SearchCriteriaParserError {
                [CCode (cname = "GUPNP_SEARCH_CRITERIA_PARSER_ERROR_FAILED")]
                FAILED
        }

	[CCode (cheader_filename = "libgupnp-av/gupnp-av.h", cprefix = "GUPNP_PROTOCOL_ERROR_")]
	public errordomain ProtocolError {
		INVALID_SYNTAX,
		OTHER;
		public static GLib.Quark quark ();
	}

        public class SearchCriteriaParser : GLib.Object {
            public virtual signal bool expression (string property, GUPnP.SearchCriteriaOp op, string value, GLib.Error error);
        }

        public class LastChangeParser : GLib.Object {
            public bool parse_last_change (uint instance_id, string last_change_xml, ...) throws GLib.Error;
        }
}
