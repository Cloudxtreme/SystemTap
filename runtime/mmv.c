/*
 * Copyright (C) 2014 Red Hat Inc.
 *
 * This file is part of systemtap, and is free software.  You can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License (GPL); either version 2, or (at your option) any
 * later version.
 */

#ifndef _MMV_C_
#define _MMV_C_

static void _stp_mmv_data_init(void *data, size_t nbytes);

#if defined(__KERNEL__)
#include "linux/mmv.c"
#elif defined(__DYNINST__)
//#include "dyninst/mmv.c"
#error "Not written yet"
#endif

//
// Storage for global instances, indoms, and metrics.
//
// FIXME: For now, no locking.
//
#define _STP_MMV_DISK_HEADER	0
#define _STP_MMV_DISK_TOC	1
#define _STP_MMV_DISK_INDOMS	2
#define _STP_MMV_DISK_INSTANCES 3
#define _STP_MMV_DISK_METRICS	4
#define _STP_MMV_DISK_VALUES	5
#define _STP_MMV_DISK_STRINGS	6

#define _STP_MMV_MAX_SECTIONS	7

struct __stp_mmv_data {
	void *data;			/* data pointer */
	size_t nbytes;			/* allocated size */
	size_t size;			/* size used */
	size_t offset[_STP_MMV_MAX_SECTIONS];
	void *ptr[_STP_MMV_MAX_SECTIONS];
	int nitems[_STP_MMV_MAX_SECTIONS];
};

static struct __stp_mmv_data _stp_mmv_data = {
	NULL, 0, 0, { 0 }, { NULL }, { 0 }
};

static void _stp_mmv_data_init(void *data, size_t nbytes)
{
	// We could think about allocating a __stp_mmv_data structure
	// here, instead of having a static one. But, right now we'll
	// only ever have one of these files.
	_stp_mmv_data.data = data;
	_stp_mmv_data.nbytes = nbytes;

	_stp_mmv_data.size = 0;
	_stp_mmv_data.offset[_STP_MMV_DISK_HEADER] = _stp_mmv_data.size;
	_stp_mmv_data.ptr[_STP_MMV_DISK_HEADER] = _stp_mmv_data.data
		+ _stp_mmv_data.size;
	_stp_mmv_data.size += sizeof(mmv_disk_header_t);

	// TOC (Table Of Contents) section. Always assume the max.
	_stp_mmv_data.offset[_STP_MMV_DISK_TOC] = _stp_mmv_data.size;
	_stp_mmv_data.ptr[_STP_MMV_DISK_TOC] = _stp_mmv_data.data
		+ _stp_mmv_data.size;
	_stp_mmv_data.size += (sizeof(mmv_disk_toc_t) * _STP_MMV_MAX_SECTIONS);

	// We have no idea how big the indoms, instances, metrics,
	// value, and string sections should be, so don't worry about
	// them now.
}

//FIXME: need to lock the mmv_data so simultaneous allocs work
//properly. Hmm, probably the caller(s) should lock.

static void *
__stp_mmv_alloc_data_item(int item_type, size_t item_size)
{
	char *ptr;

	/* If there is space enough for another data item? */
	if ((_stp_mmv_data.nbytes - _stp_mmv_data.size) < item_size)
		return ERR_PTR(-ENOMEM);

	/* If we haven't allocated any data items of this type yet,
	 * grab space from the end of the buffer. */
	if (_stp_mmv_data.ptr[item_type] == NULL) {
		_stp_mmv_data.offset[item_type] = _stp_mmv_data.size;
		_stp_mmv_data.ptr[item_type] = _stp_mmv_data.data + _stp_mmv_data.size;
		_stp_mmv_data.size += item_size;
		_stp_mmv_data.nitems[item_type]++;
		memset(_stp_mmv_data.ptr[item_type], 0, item_size);
		return (void *)_stp_mmv_data.ptr[item_type];
	}

	/* Calculate where the next data item would go. */
	ptr = ((char *)_stp_mmv_data.ptr[item_type]
		+ (_stp_mmv_data.nitems[item_type] * item_size));

	/* Are the data items at the end of the buffer? If not, we'll
	 * have to move data around (since all data items of the same
	 * type must be contiguous. */
	if ((_stp_mmv_data.offset[item_type]
	     + _stp_mmv_data.nitems[item_type] * item_size) != _stp_mmv_data.size) {
		int i;
		char *dest = ptr + item_size;
		int section_moved[_STP_MMV_MAX_SECTIONS] = { 0 };
		mmv_disk_indom_t *indom;
		mmv_disk_instance_t *instance;
		mmv_disk_metric_t *metric;
		mmv_disk_value_t *value;

		/* Move data. Notice we're using memmove() instead of
		 * memcpy(), since memmove() handles overlapping
		 * moves. */
		memmove(dest, ptr,
			(_stp_mmv_data.size
			 - (_stp_mmv_data.offset[item_type]
			    + _stp_mmv_data.nitems[item_type] * item_size)));

		/* Update all offsets, starting past the toc, since
		 * the header and toc are fixed. */
		for (i = _STP_MMV_DISK_TOC + 1; i < _STP_MMV_MAX_SECTIONS;
		     i++) {
			if (i != item_type
			    && _stp_mmv_data.offset[i] > _stp_mmv_data.offset[item_type]) {
				_stp_mmv_data.offset[i] += item_size;
				_stp_mmv_data.ptr[i] += item_size;
				section_moved[i] = 1;
			}
		}

		/* Update data offsets. Why is this necessary? All
		 * the offsets in each section are offsets from the
		 * beginning of the buffer, not the beginning of the
		 * section. */
		indom = _stp_mmv_data.ptr[_STP_MMV_DISK_INDOMS];
		for (i = 0; i < _stp_mmv_data.nitems[_STP_MMV_DISK_INDOMS];
		     i++) {
			if (section_moved[_STP_MMV_DISK_INSTANCES]
			    && indom[i].offset)
				indom[i].offset += item_size;
			if (section_moved[_STP_MMV_DISK_STRINGS]) {
				if (indom[i].shorttext)
					indom[i].shorttext += item_size;
				if (indom[i].helptext)
					indom[i].helptext += item_size;
			}
		}
		instance = _stp_mmv_data.ptr[_STP_MMV_DISK_INSTANCES];
		for (i = 0; i < _stp_mmv_data.nitems[_STP_MMV_DISK_INSTANCES];
		     i++) {
			if (section_moved[_STP_MMV_DISK_INDOMS]
			    && instance[i].indom)
				instance[i].indom += item_size;
		}
		metric = _stp_mmv_data.ptr[_STP_MMV_DISK_METRICS];
		for (i = 0; i < _stp_mmv_data.nitems[_STP_MMV_DISK_METRICS];
		     i++) {
			if (section_moved[_STP_MMV_DISK_STRINGS]) {
				if (metric[i].shorttext)
					metric[i].shorttext += item_size;
				if (metric[i].helptext)
					metric[i].helptext += item_size;
			}
		}
		value = _stp_mmv_data.ptr[_STP_MMV_DISK_VALUES];
		for (i = 0; i < _stp_mmv_data.nitems[_STP_MMV_DISK_VALUES];
		     i++) {
			if (section_moved[_STP_MMV_DISK_METRICS]
			    && value[i].metric)
				value[i].metric += item_size;
			if (section_moved[_STP_MMV_DISK_INSTANCES]
			    && value[i].instance)
				value[i].instance += item_size;
		}
	}
	
	/* Update bookkeeping. */
	_stp_mmv_data.size += item_size;
	_stp_mmv_data.nitems[item_type]++;

	/* Clear everything out. */
	memset(ptr, 0, item_size);
	return (void *)ptr;
}

static int
__stp_mmv_add_instance(int32_t internal, char *external)
{
	mmv_disk_instance_t *instance = (mmv_disk_instance_t *)__stp_mmv_alloc_data_item(_STP_MMV_DISK_INSTANCES, sizeof(mmv_disk_instance_t));

	if (IS_ERR(instance))
		return PTR_ERR(instance);

	// Real 'indom' value gets filled in later when the instance
	// gets added to the indom.
	instance->indom = 0;
	instance->padding = 0;
	instance->internal = internal;
	strncpy (instance->external, external, MMV_NAMEMAX);
	return (_stp_mmv_data.nitems[_STP_MMV_DISK_INSTANCES] - 1);
}

static int
__stp_mmv_add_string(char *str)
{
	mmv_disk_string_t *string;

	/* Note that this function returns 0 for "no string needed"
	 * and for "the 1st string offset". So, callers should be
	 * careful. */
	if (str == NULL || strlen(str) == 0)
		return 0;

	string = (mmv_disk_string_t *)__stp_mmv_alloc_data_item(_STP_MMV_DISK_STRINGS, sizeof(mmv_disk_string_t));
	if (IS_ERR(string))
		return PTR_ERR(string);

	strncpy(string->payload, str, MMV_STRINGMAX);
	string->payload[MMV_STRINGMAX-1] = '\0';
	return (_stp_mmv_data.nitems[_STP_MMV_DISK_STRINGS] - 1);
}

static int
__stp_mmv_add_indom(uint32_t serial, char *shorttext, char *helptext)
{
	int shorttext_idx = 0;
	int helptext_idx = 0;
	mmv_disk_indom_t *indom;

	/* Notice we're adding the strings first. This way the pointer
	 * returned by __stp_mmv_alloc_data_item() will remain
	 * valid. If we called __stp_mmv_add_string() later, that
	 * could cause some memory moving, invalidating the
	 * pointer. */
	if (shorttext != NULL && strlen(shorttext))
		shorttext_idx = __stp_mmv_add_string(shorttext);
	if (helptext != NULL && strlen(helptext))
		helptext_idx = __stp_mmv_add_string(helptext);
	if (shorttext_idx < 0 || helptext_idx < 0)
		return -ENOMEM;

	indom = (mmv_disk_indom_t *)__stp_mmv_alloc_data_item(_STP_MMV_DISK_INDOMS, sizeof(mmv_disk_indom_t));
	if (IS_ERR(indom))
		return PTR_ERR(indom);

	indom->serial = serial;
	indom->count = 0;
	indom->offset = 0;
	if (shorttext != NULL && strlen(shorttext))
		indom->shorttext = (_stp_mmv_data.offset[_STP_MMV_DISK_STRINGS]
				    + (shorttext_idx * sizeof(mmv_disk_string_t)));
	if (helptext != NULL && strlen(helptext))
		indom->helptext = (_stp_mmv_data.offset[_STP_MMV_DISK_STRINGS]
				   + (helptext_idx * sizeof(mmv_disk_string_t)));
	return (_stp_mmv_data.nitems[_STP_MMV_DISK_INDOMS] - 1);
}

// FIXME: I think we have a problem here. All the instances added to
// an indom must be consecutive. We'll either have to error if they
// are not or rearrange things behind the scene.

static int
__stp_mmv_add_indom_instance(int indom_idx, int instance_idx)
{
	mmv_disk_indom_t *indom = (mmv_disk_indom_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_INDOMS];
	mmv_disk_instance_t *instance = (mmv_disk_instance_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_INSTANCES];
	uint64_t instance_offset;

	if (indom_idx >= _stp_mmv_data.nitems[_STP_MMV_DISK_INDOMS]
	    || instance_idx >= _stp_mmv_data.nitems[_STP_MMV_DISK_INSTANCES]) {
		// Error message provided by caller.
		return -EINVAL;
	}

	indom[indom_idx].count++;
	instance_offset = _stp_mmv_data.offset[_STP_MMV_DISK_INSTANCES]
		+ (instance_idx * sizeof(mmv_disk_instance_t));
	if (indom[indom_idx].offset == 0
	    || indom[indom_idx].offset > instance_offset)
		indom[indom_idx].offset = instance_offset;
	instance[instance_idx].indom = _stp_mmv_data.offset[_STP_MMV_DISK_INDOMS]
		+ (indom_idx * sizeof(mmv_disk_indom_t));
		
	return 0;
}

static mmv_disk_indom_t *
__stp_mmv_lookup_disk_indom(int serial)
{
	mmv_disk_indom_t *indom = (mmv_disk_indom_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_INDOMS];
	int i;

	for (i = 0; i < _stp_mmv_data.nitems[_STP_MMV_DISK_INDOMS]; i++) {
		if (indom[i].serial == serial)
			return &indom[i];
	}
	return NULL;
}

static int
__stp_mmv_add_metric(char *name, uint32_t item, mmv_metric_type_t type,
		     mmv_metric_sem_t semantics, mmv_units_t dimension,
		     uint32_t indom_serial, char *shorttext, char *helptext)
{
	int shorttext_idx = 0;
	int helptext_idx = 0;
	mmv_disk_metric_t *metric;
	mmv_disk_value_t *value;
	uint64_t metric_offset;

	/* Notice we're adding the strings first. This way the pointer
	 * returned by __stp_mmv_alloc_data_item() will remain
	 * valid. If we called __stp_mmv_add_string() later, that
	 * could cause some memory moving, invalidating the
	 * pointer. */
	if (shorttext != NULL && strlen(shorttext))
		shorttext_idx = __stp_mmv_add_string(shorttext);
	if (helptext != NULL && strlen(helptext))
		helptext_idx = __stp_mmv_add_string(helptext);
	if (shorttext_idx < 0 || helptext_idx < 0)
		return -ENOMEM;
	
	metric = (mmv_disk_metric_t *)__stp_mmv_alloc_data_item(_STP_MMV_DISK_METRICS, sizeof(mmv_disk_metric_t));
	if (IS_ERR(metric))
		return PTR_ERR(metric);

	strncpy(metric->name, name, MMV_NAMEMAX);
	metric->item = item;
	metric->type = type;
	metric->semantics = semantics;
	metric->dimension = dimension;
	metric->indom = indom_serial;
	if (shorttext != NULL && strlen(shorttext))
		metric->shorttext = (_stp_mmv_data.offset[_STP_MMV_DISK_STRINGS]
				     + (shorttext_idx * sizeof(mmv_disk_string_t)));
	if (helptext != NULL && strlen(helptext))
		metric->helptext = (_stp_mmv_data.offset[_STP_MMV_DISK_STRINGS]
				    + (helptext_idx * sizeof(mmv_disk_string_t)));

	/* This offset is relative to the beginning of the metric
	 * section. We can't add in the offset from the beginning of
	 * the data buffer since the start of the metric section might
	 * move around when allocating new items. */
	metric_offset = ((_stp_mmv_data.nitems[_STP_MMV_DISK_METRICS] - 1)
			 * sizeof(mmv_disk_metric_t));
	if (metric->indom == 0) {
		value = (mmv_disk_value_t *)__stp_mmv_alloc_data_item(_STP_MMV_DISK_VALUES, sizeof(mmv_disk_value_t));
		if (IS_ERR(value))
			return PTR_ERR(value);

		value->metric = (_stp_mmv_data.offset[_STP_MMV_DISK_METRICS]
				 + metric_offset);
		value->instance = 0;
	}
	else {
		mmv_disk_indom_t *indom = __stp_mmv_lookup_disk_indom(indom_serial);
		int i;
		uint32_t indom_count;
		int value_idx = -1;
		
		if (indom == NULL) {
			// Error message provided by caller.
			return -EINVAL;
		}
		
		/* Notice we're caching the indom count from the indom
		 * pointer. Since we're allocating values, that
		 * pointer could become invalid. */
		indom_count = indom->count;
		for (i = 0; i < indom_count; i++) {
			value = (mmv_disk_value_t *)__stp_mmv_alloc_data_item(_STP_MMV_DISK_VALUES, sizeof(mmv_disk_value_t));
			if (IS_ERR(value))
				return PTR_ERR(value);

			/* Remember starting value index. */
			if (value_idx < 0)
				value_idx = _stp_mmv_data.nitems[_STP_MMV_DISK_VALUES] - 1;
		}			
		/* Now that all the values are allocated, pointer
		 * values and offsets should remain valid. */
		value = (mmv_disk_value_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_VALUES];
		indom = __stp_mmv_lookup_disk_indom(indom_serial);
		for (i = value_idx; i < (indom_count + value_idx); i++) {
			value[i].metric = (_stp_mmv_data.offset[_STP_MMV_DISK_METRICS]
					   + metric_offset);
			value[i].instance = indom->offset;
		}
	}
	return (_stp_mmv_data.nitems[_STP_MMV_DISK_METRICS] - 1);
}

static int
__stp_mmv_stats_init(int cluster, mmv_stats_flags_t flags)
{
	mmv_disk_header_t *hdr = (mmv_disk_header_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_HEADER];
	mmv_disk_toc_t *toc = (mmv_disk_toc_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_TOC];
	int tocidx = 0;

	strcpy(hdr->magic, "MMV");
	hdr->version = 1;
	hdr->g1 = _stp_random_u(999999);
	hdr->g2 = 0;
	hdr->tocs = 2;
	if (_stp_mmv_data.nitems[_STP_MMV_DISK_INDOMS] > 0)
		hdr->tocs += 2;
	if (_stp_mmv_data.nitems[_STP_MMV_DISK_STRINGS])
		hdr->tocs++;

	hdr->flags = flags;
	hdr->process = 0; //?
	hdr->cluster = cluster;

	if (_stp_mmv_data.nitems[_STP_MMV_DISK_INDOMS]) {
		toc[tocidx].type = MMV_TOC_INDOMS;
		toc[tocidx].count = _stp_mmv_data.nitems[_STP_MMV_DISK_INDOMS];
		toc[tocidx].offset = _stp_mmv_data.offset[_STP_MMV_DISK_INDOMS];
		tocidx++;
		toc[tocidx].type = MMV_TOC_INSTANCES;
		toc[tocidx].count = _stp_mmv_data.nitems[_STP_MMV_DISK_INSTANCES];
		toc[tocidx].offset = _stp_mmv_data.offset[_STP_MMV_DISK_INSTANCES];
		tocidx++;
	}
	toc[tocidx].type = MMV_TOC_METRICS;
	toc[tocidx].count = _stp_mmv_data.nitems[_STP_MMV_DISK_METRICS];
	toc[tocidx].offset = _stp_mmv_data.offset[_STP_MMV_DISK_METRICS];
	tocidx++;
	toc[tocidx].type = MMV_TOC_VALUES;
	toc[tocidx].count = _stp_mmv_data.nitems[_STP_MMV_DISK_VALUES];
	toc[tocidx].offset = _stp_mmv_data.offset[_STP_MMV_DISK_VALUES];
	tocidx++;
	if (_stp_mmv_data.nitems[_STP_MMV_DISK_STRINGS]) {
		toc[tocidx].type = MMV_TOC_STRINGS;
		toc[tocidx].count = _stp_mmv_data.nitems[_STP_MMV_DISK_STRINGS];
		toc[tocidx].offset = _stp_mmv_data.offset[_STP_MMV_DISK_STRINGS];
		tocidx++;
	}

	// Setting g2 to the same value as g1 lets clients know that
	// we are finished initializing.
	hdr->g2 = hdr->g1;
	return 0;
}

static int
__stp_mmv_stats_stop(void)
{
	return 0;
}

static int
__stp_mmv_lookup_value(int metric_idx, int instance_idx)
{
	mmv_disk_value_t *value = (mmv_disk_value_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_VALUES];
	mmv_disk_metric_t *metric = (mmv_disk_metric_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_METRICS];
	uint64_t metric_offset = _stp_mmv_data.offset[_STP_MMV_DISK_METRICS]
		+ (sizeof(mmv_disk_metric_t) * metric_idx);
	uint64_t instance_offset;
	int i;

	if (metric_idx < 0
	    || metric_idx >= _stp_mmv_data.nitems[_STP_MMV_DISK_METRICS])
		return -EINVAL;
	if (instance_idx != 0
	    && (instance_idx < 0
		|| instance_idx >= _stp_mmv_data.nitems[_STP_MMV_DISK_INSTANCES]))
		return -EINVAL;
	instance_offset = _stp_mmv_data.offset[_STP_MMV_DISK_INSTANCES]
		+ (sizeof(mmv_disk_instance_t) * instance_idx);
	for (i = 0; i < _stp_mmv_data.nitems[_STP_MMV_DISK_VALUES]; i++) {
		if (value[i].metric == metric_offset) {
			/* Singular metric */
			if (metric[metric_idx].indom == 0) {
				return i;
			}
			else if (value[i].instance == instance_offset) {
				return i;
			}
		}
	}
	return -EINVAL;
}

static int
__stp_mmv_set_value(int value_idx, uint64_t value)
{
	mmv_disk_value_t *v = (mmv_disk_value_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_VALUES];
	mmv_disk_metric_t *m;

	if (value_idx < 0
	    || value_idx >= _stp_mmv_data.nitems[_STP_MMV_DISK_VALUES]) {
		return -EINVAL;
	}

	m = (mmv_disk_metric_t *)((char *)_stp_mmv_data.data + v[value_idx].metric);
	if (m->type != MMV_TYPE_I64) {
		return -EINVAL;
	}
	v[value_idx].value.ll = value;
	return 0;
}

static int
__stp_mmv_inc_value(int value_idx, uint64_t inc)
{
	mmv_disk_value_t *v = (mmv_disk_value_t *)_stp_mmv_data.ptr[_STP_MMV_DISK_VALUES];
	mmv_disk_metric_t *m;

	if (value_idx < 0
	    || value_idx >= _stp_mmv_data.nitems[_STP_MMV_DISK_VALUES]) {
		return -EINVAL;
	}

	m = (mmv_disk_metric_t *)((char *)_stp_mmv_data.data + v[value_idx].metric);
	if (m->type != MMV_TYPE_I64) {
		return -EINVAL;
	}
	v[value_idx].value.ll += inc;
	return 0;
}

#endif /* _MMV_C_ */
