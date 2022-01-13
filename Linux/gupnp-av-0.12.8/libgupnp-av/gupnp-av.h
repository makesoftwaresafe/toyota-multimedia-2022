/*
 * Copyright (C) 2007 Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 * Copyright (C) 2006, 2007, 2008 OpenedHand Ltd.
 *
 * Author: Zeeshan Ali Khattak <zeenix@gstreamer.net>
 *         Jorn Baayen <jorn@openedhand.com>
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

#include "gupnp-av-enums.h"
#include "gupnp-didl-lite-object.h"
#include "gupnp-didl-lite-container.h"
#include "gupnp-didl-lite-createclass.h"
#include "gupnp-didl-lite-item.h"
#include "gupnp-didl-lite-parser.h"
#include "gupnp-didl-lite-resource.h"
#include "gupnp-didl-lite-descriptor.h"
#include "gupnp-didl-lite-writer.h"
#include "gupnp-protocol-info.h"
#include "gupnp-search-criteria-parser.h"
#include "gupnp-last-change-parser.h"
#include "gupnp-cds-last-change-parser.h"
#include "gupnp-feature.h"
#include "gupnp-feature-list-parser.h"
#include "gupnp-dlna.h"
#include "gupnp-media-collection.h"
