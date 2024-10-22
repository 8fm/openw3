/*
 * Copyright 2011-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to NVIDIA intellectual property rights under U.S. and
 * international Copyright laws.
 *
 * These Licensed Deliverables contained herein is PROPRIETARY and
 * CONFIDENTIAL to NVIDIA and is being provided under the terms and
 * conditions of a form of NVIDIA software license agreement by and
 * between NVIDIA and Licensee ("License Agreement") or electronically
 * accepted by Licensee.  Notwithstanding any terms or conditions to
 * the contrary in the License Agreement, reproduction or disclosure
 * of the Licensed Deliverables to any third party without the express
 * written consent of NVIDIA is prohibited.
 *
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, NVIDIA MAKES NO REPRESENTATION ABOUT THE
 * SUITABILITY OF THESE LICENSED DELIVERABLES FOR ANY PURPOSE.  IT IS
 * PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.
 * NVIDIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THESE LICENSED
 * DELIVERABLES, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
 * NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY
 * SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THESE LICENSED DELIVERABLES.
 *
 * U.S. Government End Users.  These Licensed Deliverables are a
 * "commercial item" as that term is defined at 48 C.F.R. 2.101 (OCT
 * 1995), consisting of "commercial computer software" and "commercial
 * computer software documentation" as such terms are used in 48
 * C.F.R. 12.212 (SEPT 1995) and is provided to the U.S. Government
 * only as a commercial end item.  Consistent with 48 C.F.R.12.212 and
 * 48 C.F.R. 227.7202-1 through 227.7202-4 (JUNE 1995), all
 * U.S. Government End Users acquire the Licensed Deliverables with
 * only those rights set forth herein.
 *
 * Any use of the Licensed Deliverables in individual and commercial
 * software must include, in the user documentation and internal
 * comments to the code, the above Disclaimer and U.S. Government End
 * Users Notice.
 */

#if !defined(_CUPTI_METRIC_H_)
#define _CUPTI_METRIC_H_

#include <cuda.h>
#include <string.h>
#include <cuda_stdint.h>
#include <cupti_result.h>

#ifndef CUPTIAPI
#ifdef _WIN32
#define CUPTIAPI __stdcall
#else 
#define CUPTIAPI
#endif 
#endif 

#if defined(__cplusplus)
extern "C" {
#endif 

/**
 * \defgroup CUPTI_METRIC_API CUPTI Metric API
 * Functions, types, and enums that implement the CUPTI Metric API.
 * @{
 */

/**
 * \brief ID for a metric.
 *
 * A metric provides a measure of some aspect of the device. 
 */
typedef uint32_t CUpti_MetricID;

/**
 * \brief A metric category.
 * 
 * Each metric is assigned to a category that represents the general
 * type of the metric. A metric's category is accessed using \ref
 * cuptiMetricGetAttribute and the CUPTI_METRIC_ATTR_CATEGORY
 * attribute.
 */
typedef enum {
  /**
   * A memory related metric.
   */
  CUPTI_METRIC_CATEGORY_MEMORY          = 0,
  /**
   * An instruction related metric.
   */
  CUPTI_METRIC_CATEGORY_INSTRUCTION     = 1,
  /**
   * A multiprocessor related metric.
   */
  CUPTI_METRIC_CATEGORY_MULTIPROCESSOR  = 2,
  /**
   * A cache related metric.
   */
  CUPTI_METRIC_CATEGORY_CACHE           = 3,
  /**
   * A texture related metric.
   */
  CUPTI_METRIC_CATEGORY_TEXTURE         = 4,
  CUPTI_METRIC_CATEGORY_FORCE_INT                         = 0x7fffffff,
} CUpti_MetricCategory;

/**
 * \brief A metric evaluation mode.
 * 
 * A metric can be evaluated per hardware instance to know the load balancing
 * across instances of a domain or the metric can be evaluated in aggregate mode
 * when the events involved in metric evaluation are from different event
 * domains. It might be possible to evaluate some metrics in both
 * modes for convenience. A metric's evaluation mode is accessed using \ref
 * CUpti_MetricEvaluationMode and the CUPTI_METRIC_ATTR_EVALUATION_MODE
 * attribute.
 */
typedef enum {
  /**
   * If the metric evaluation mode is per instance, then the event value passed
   * to \ref cuptiMetricGetValue should contain value for an instance of the
   * domain. Also in this mode, \ref cuptiMetricGetValue should be called for all
   * available instances of the domain to get overall status.
   */
  CUPTI_METRIC_EVALUATION_MODE_PER_INSTANCE         = 1,
  /**
   * If the metric evaluation mode is aggregate, then the event value passed
   * to \ref cuptiMetricGetValue should be aggregated value of an event
   * for all instances of the domain.
   * In this mode, \ref cuptiMetricGetValue should be called only once.
   */
  CUPTI_METRIC_EVALUATION_MODE_AGGREGATE            = 1 << 1,
  CUPTI_METRIC_EVALUATION_MODE_FORCE_INT            = 0x7fffffff,
} CUpti_MetricEvaluationMode;

/**
 * \brief Kinds of metric values.
 *
 * Metric values can be one of several different kinds. Corresponding
 * to each kind is a member of the CUpti_MetricValue union. The metric
 * value returned by \ref cuptiMetricGetValue should be accessed using
 * the appropriate member of that union based on its value kind.
 */
typedef enum {
  /**
   * The metric value is a 64-bit double.
   */
  CUPTI_METRIC_VALUE_KIND_DOUBLE     = 0,
  /**
   * The metric value is a 64-bit unsigned integer.
   */
  CUPTI_METRIC_VALUE_KIND_UINT64     = 1,
  /**
   * The metric value is a percentage represented by a 64-bit
   * double. For example, 57.5% is represented by the value 57.5.
   */
  CUPTI_METRIC_VALUE_KIND_PERCENT    = 2,
  /**
   * The metric value is a throughput represented by a 64-bit
   * integer. The unit for throughput values is bytes/second.
   */
  CUPTI_METRIC_VALUE_KIND_THROUGHPUT = 3,
  /**
   * The metric value is a 64-bit signed integer.
   */
  CUPTI_METRIC_VALUE_KIND_INT64      = 4,

  CUPTI_METRIC_VALUE_KIND_FORCE_INT  = 0x7fffffff
} CUpti_MetricValueKind;

/**
 * \brief Metric attributes.
 * 
 * Metric attributes describe properties of a metric. These attributes
 * can be read using \ref cuptiMetricGetAttribute.
 */
typedef enum {
  /**
   * Metric name. Value is a null terminated const c-string.
   */
  CUPTI_METRIC_ATTR_NAME              = 0,
  /**
   * Short description of metric. Value is a null terminated const c-string.
   */
  CUPTI_METRIC_ATTR_SHORT_DESCRIPTION = 1,
  /**
   * Long description of metric. Value is a null terminated const c-string.
   */
  CUPTI_METRIC_ATTR_LONG_DESCRIPTION  = 2,
  /**
   * Category of the metric. Value is of type CUpti_MetricCategory.
   */
  CUPTI_METRIC_ATTR_CATEGORY          = 3,
  /**
   * Value type of the metric. Value is of type CUpti_MetricValueKind.
   */
  CUPTI_METRIC_ATTR_VALUE_KIND          = 4,
  /**
   * Metric evaluation mode. Value is of type CUpti_MetricEvaluationMode.
   */
  CUPTI_METRIC_ATTR_EVALUATION_MODE     = 5,
  CUPTI_METRIC_ATTR_FORCE_INT         = 0x7fffffff,
} CUpti_MetricAttribute;

/**
 * \brief A metric value.
 *
 * Metric values can be one of several different kinds. Corresponding
 * to each kind is a member of the CUpti_MetricValue union. The metric
 * value returned by \ref cuptiMetricGetValue should be accessed using
 * the appropriate member of that union based on its value kind.
 */
typedef union {
  /*
   * Value for CUPTI_METRIC_VALUE_KIND_DOUBLE.
   */
  double metricValueDouble;
  /*
   * Value for CUPTI_METRIC_VALUE_KIND_UINT64.
   */
  uint64_t metricValueUint64;
  /*
   * Value for CUPTI_METRIC_VALUE_KIND_INT64.
   */
  int64_t metricValueInt64;
  /*
   * Value for CUPTI_METRIC_VALUE_KIND_PERCENT. For example, 57.5% is
   * represented by the value 57.5.
   */
  double metricValuePercent;
  /*
   * Value for CUPTI_METRIC_VALUE_KIND_THROUGHPUT.  The unit for
   * throughput values is bytes/second.
   */
  uint64_t metricValueThroughput;
} CUpti_MetricValue;


/**
 * \brief Get the total number of metrics available on any device.
 *
 * Returns the total number of metrics available on any CUDA-capable
 * devices.
 *
 * \param numMetrics Returns the number of metrics
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p numMetrics is NULL
*/
CUptiResult CUPTIAPI cuptiGetNumMetrics(uint32_t *numMetrics);

/**
 * \brief Get all the metrics available on any device.
 *
 * Returns the metric IDs in \p metricArray for all CUDA-capable
 * devices.  The size of the \p metricArray buffer is given by \p
 * *arraySizeBytes. The size of the \p metricArray buffer must be at
 * least \p numMetrics * sizeof(CUpti_MetricID) or all metric IDs will
 * not be returned. The value returned in \p *arraySizeBytes contains
 * the number of bytes returned in \p metricArray.
 *
 * \param arraySizeBytes The size of \p metricArray in bytes, and
 * returns the number of bytes written to \p metricArray
 * \param metricArray Returns the IDs of the metrics
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p arraySizeBytes or
 * \p metricArray are NULL
*/
CUptiResult CUPTIAPI cuptiEnumMetrics(size_t *arraySizeBytes, 
                                      CUpti_MetricID *metricArray);

/**
 * \brief Get the number of metrics for a device.
 *
 * Returns the number of metrics available for a device.
 *
 * \param device The CUDA device
 * \param numMetrics Returns the number of metrics available for the
 * device
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_DEVICE
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p numMetrics is NULL
 */
CUptiResult CUPTIAPI cuptiDeviceGetNumMetrics(CUdevice device, 
                                              uint32_t *numMetrics);

/**
 * \brief Get the metrics for a device.
 *
 * Returns the metric IDs in \p metricArray for a device.  The size of
 * the \p metricArray buffer is given by \p *arraySizeBytes. The size
 * of the \p metricArray buffer must be at least \p numMetrics *
 * sizeof(CUpti_MetricID) or else all metric IDs will not be
 * returned. The value returned in \p *arraySizeBytes contains the
 * number of bytes returned in \p metricArray.
 *
 * \param device The CUDA device
 * \param arraySizeBytes The size of \p metricArray in bytes, and
 * returns the number of bytes written to \p metricArray
 * \param metricArray Returns the IDs of the metrics for the device
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_DEVICE
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p arraySizeBytes or 
 * \p metricArray are NULL
 */
CUptiResult CUPTIAPI cuptiDeviceEnumMetrics(CUdevice device, 
                                            size_t *arraySizeBytes, 
                                            CUpti_MetricID *metricArray);

/**
 * \brief Get a metric attribute.
 *
 * Returns a metric attribute in \p *value. The size of the \p
 * value buffer is given by \p *valueSize. The value returned in \p
 * *valueSize contains the number of bytes returned in \p value.
 *
 * If the attribute value is a c-string that is longer than \p
 * *valueSize, then only the first \p *valueSize characters will be
 * returned and there will be no terminating null byte.
 *
 * \param metric ID of the metric
 * \param attrib The metric attribute to read
 * \param valueSize The size of the \p value buffer in bytes, and
 * returns the number of bytes written to \p value
 * \param value Returns the attribute's value
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_METRIC_ID
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p valueSize or \p value
 * is NULL, or if \p attrib is not a metric attribute
 * \retval CUPTI_ERROR_PARAMETER_SIZE_NOT_SUFFICIENT For non-c-string
 * attribute values, indicates that the \p value buffer is too small
 * to hold the attribute value.
 */
CUptiResult CUPTIAPI cuptiMetricGetAttribute(CUpti_MetricID metric, 
                                             CUpti_MetricAttribute attrib, 
                                             size_t *valueSize, 
                                             void *value);

/**
 * \brief Find an metric by name.
 *
 * Find a metric by name and return the metric ID in \p *metric.
 *
 * \param device The CUDA device
 * \param metricName The name of metric to find
 * \param metric Returns the ID of the found metric or undefined if
 * unable to find the metric
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_DEVICE
 * \retval CUPTI_ERROR_INVALID_METRIC_NAME if unable to find a metric
 * with name \p metricName. In this case \p *metric is undefined
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p metricName or \p
 * metric are NULL.
 */
CUptiResult CUPTIAPI cuptiMetricGetIdFromName(CUdevice device, 
                                              const char *metricName, 
                                              CUpti_MetricID *metric);

/**
 * \brief Get number of events required to calculate a metric.
 *
 * Returns the number of events in \p numEvents that are required to
 * calculate a metric.
 *
 * \param metric ID of the metric
 * \param numEvents Returns the number of events required for the metric
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_METRIC_ID
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p numEvents is NULL
 */
CUptiResult CUPTIAPI cuptiMetricGetNumEvents(CUpti_MetricID metric, 
                                             uint32_t *numEvents);

/**
 * \brief Get the events required to calculating a metric.
 *
 * Gets the event IDs in \p eventIdArray required to calculate a \p
 * metric. The size of the \p eventIdArray buffer is given by \p
 * *eventIdArraySizeBytes and must be at least \p numEvents *
 * sizeof(CUpti_EventID) or all events will not be returned. The value
 * returned in \p *eventIdArraySizeBytes contains the number of bytes
 * returned in \p eventIdArray.
 *
 * \param metric ID of the metric
 * \param eventIdArraySizeBytes The size of \p eventIdArray in bytes,
 * and returns the number of bytes written to \p eventIdArray
 * \param eventIdArray Returns the IDs of the events required to
 * calculate \p metric
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_METRIC_ID
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p eventIdArraySizeBytes or \p
 * eventIdArray are NULL.
 */
CUptiResult CUPTIAPI cuptiMetricEnumEvents(CUpti_MetricID metric, 
                                           size_t *eventIdArraySizeBytes, 
                                           CUpti_EventID *eventIdArray);

/**
 * \brief For a set of metrics, get the grouping that indicates the
 * number of passes and the event groups necessary to collect the
 * events required for those metrics.
 *
 * For a set of metrics, get the grouping that indicates the number of
 * passes and the event groups necessary to collect the events
 * required for those metrics. 
 *
 * \see cuptiEventGroupSetsCreate for details on event group set
 * creation.
 *
 * \param context The context for event collection
 * \param metricIdArraySizeBytes Size of the metricIdArray in bytes
 * \param metricIdArray Array of metric IDs
 * \param eventGroupPasses Returns a CUpti_EventGroupSets object that
 * indicates the number of passes required to collect the events and
 * the events to collect on each pass
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_CONTEXT
 * \retval CUPTI_ERROR_INVALID_METRIC_ID
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p metricIdArray or
 * \p eventGroupPasses is NULL
 */
CUptiResult CUPTIAPI cuptiMetricCreateEventGroupSets(CUcontext context,
                                                     size_t metricIdArraySizeBytes,
                                                     CUpti_MetricID *metricIdArray,
                                                     CUpti_EventGroupSets **eventGroupPasses);

/**
 * \brief Calculate the value for a metric.
 *
 * Use the events collected for a metric to calculate the metric
 * value. Metric value evaluation depends on the evaluation mode
 * \ref CUpti_MetricEvaluationMode that the metric supports. 
 * If a metric has evaluation mode as CUPTI_METRIC_EVALUATION_MODE_PER_INSTANCE,
 * then it assumes that the input event value is for one domain instance.
 * If a metric has evaluation mode as CUPTI_METRIC_EVALUATION_MODE_AGGREGATE,
 * it assumes that input event values are
 * normalized to represent all domain instances on a device. For the
 * most accurate metric collection, the events required for the metric
 * should be collected for all profiled domain instances. For example,
 * to collect all instances of an event, set the
 * CUPTI_EVENT_GROUP_ATTR_PROFILE_ALL_DOMAIN_INSTANCES attribute on
 * the group containing the event to 1. The normalized value for the
 * event is then: (\p sum_event_values * \p totalInstanceCount) / \p
 * instanceCount, where \p sum_event_values is the summation of the
 * event values across all profiled domain instances, \p
 * totalInstanceCount is obtained from querying
 * CUPTI_EVENT_DOMAIN_ATTR_TOTAL_INSTANCE_COUNT and \p instanceCount
 * is obtained from querying CUPTI_EVENT_GROUP_ATTR_INSTANCE_COUNT (or
 * CUPTI_EVENT_DOMAIN_ATTR_INSTANCE_COUNT).
 *
 * \param device The CUDA device that the metric is being calculated for
 * \param metric The metric ID
 * \param eventIdArraySizeBytes The size of \p eventIdArray in bytes
 * \param eventIdArray The event IDs required to calculate \p metric
 * \param eventValueArraySizeBytes The size of \p eventValueArray in bytes
 * \param eventValueArray The normalized event values required to
 * calculate \p metric. The values must be order to match the order of
 * events in \p eventIdArray
 * \param timeDuration The duration over which the events were
 * collected, in ns
 * \param metricValue Returns the value for the metric
 *
 * \retval CUPTI_SUCCESS
 * \retval CUPTI_ERROR_NOT_INITIALIZED
 * \retval CUPTI_ERROR_INVALID_METRIC_ID
 * \retval CUPTI_ERROR_INVALID_OPERATION
 * \retval CUPTI_ERROR_PARAMETER_SIZE_NOT_SUFFICIENT if the
 * eventIdArray does not contain all the events needed for metric
 * \retval CUPTI_ERROR_INVALID_EVENT_VALUE if any of the
 * event values required for the metric is CUPTI_EVENT_OVERFLOW
 * \retval CUPTI_ERROR_NOT_COMPATIBLE if the computed metric value
 * cannot be represented in the metric's value type. For example,
 * if the metric value type is unsigned and the computed metric value is negative
 * \retval CUPTI_ERROR_INVALID_PARAMETER if \p metricValue,
 * \p eventIdArray or \p eventValueArray is NULL
 */
CUptiResult CUPTIAPI cuptiMetricGetValue(CUdevice device, 
                                         CUpti_MetricID metric,
                                         size_t eventIdArraySizeBytes,
                                         CUpti_EventID *eventIdArray,
                                         size_t eventValueArraySizeBytes,
                                         uint64_t *eventValueArray,
                                         uint64_t timeDuration,
                                         CUpti_MetricValue *metricValue);

/** @} */ /* END CUPTI_METRIC_API */

#if defined(__cplusplus)
}
#endif 

#endif /*_CUPTI_METRIC_H_*/


