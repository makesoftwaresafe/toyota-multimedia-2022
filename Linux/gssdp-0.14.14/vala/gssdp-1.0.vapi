/* gssdp-1.0.vapi generated by vapigen, do not modify. */

[CCode (cprefix = "GSSDP", gir_namespace = "GSSDP", gir_version = "1.0", lower_case_cprefix = "gssdp_")]
namespace GSSDP {
	[CCode (cheader_filename = "libgssdp/gssdp.h", type_id = "gssdp_client_get_type ()")]
	public class Client : GLib.Object, GLib.Initable {
		[CCode (has_construct_function = false)]
		public Client (GLib.MainContext? main_context, string? iface) throws GLib.Error;
		public void append_header (string name, string value);
		public void clear_headers ();
		public bool get_active ();
		public unowned string get_host_ip ();
		public unowned string get_interface ();
		public GLib.MainContext get_main_context ();
		public unowned string get_network ();
		public unowned string get_server_id ();
		public void remove_header (string name);
		public void set_network (string network);
		public void set_server_id (string server_id);
		[CCode (has_construct_function = false)]
		public Client.with_port (string? iface, uint16 msearch_port) throws GLib.Error;
		[NoAccessorMethod]
		public bool active { get; set; }
		[NoAccessorMethod]
		public string host_ip { owned get; set construct; }
		public string @interface { get; construct; }
		[NoAccessorMethod]
		public uint msearch_port { get; construct; }
		public string network { get; set construct; }
		public string server_id { get; set; }
		[NoAccessorMethod]
		public uint socket_ttl { get; construct; }
	}
	[CCode (cheader_filename = "libgssdp/gssdp.h", type_id = "gssdp_resource_browser_get_type ()")]
	public class ResourceBrowser : GLib.Object {
		[CCode (has_construct_function = false)]
		public ResourceBrowser (GSSDP.Client client, string target);
		public bool get_active ();
		public unowned GSSDP.Client get_client ();
		public ushort get_mx ();
		public unowned string get_target ();
		public bool rescan ();
		public void set_active (bool active);
		public void set_mx (ushort mx);
		public void set_target (string target);
		public bool active { get; set; }
		public GSSDP.Client client { get; construct; }
		public uint mx { get; set; }
		public string target { get; set; }
		public signal void resource_available (string usn, GLib.List<string> locations);
		public virtual signal void resource_unavailable (string usn);
	}
	[CCode (cheader_filename = "libgssdp/gssdp.h", type_id = "gssdp_resource_group_get_type ()")]
	public class ResourceGroup : GLib.Object {
		[CCode (has_construct_function = false)]
		public ResourceGroup (GSSDP.Client client);
		public uint add_resource (string target, string usn, GLib.List<string> locations);
		public uint add_resource_simple (string target, string usn, string location);
		public bool get_available ();
		public unowned GSSDP.Client get_client ();
		public uint get_max_age ();
		public uint get_message_delay ();
		public void remove_resource (uint resource_id);
		public void set_available (bool available);
		public void set_max_age (uint max_age);
		public void set_message_delay (uint message_delay);
		public bool available { get; set; }
		public GSSDP.Client client { get; construct; }
		public uint max_age { get; set; }
		public uint message_delay { get; set; }
	}
	[CCode (cheader_filename = "libgssdp/gssdp.h", cprefix = "GSSDP_ERROR_")]
	public errordomain Error {
		NO_IP_ADDRESS,
		FAILED;
		public static GLib.Quark quark ();
	}
	[CCode (cheader_filename = "libgssdp/gssdp.h", cname = "GSSDP_ALL_RESOURCES")]
	public const string ALL_RESOURCES;
}