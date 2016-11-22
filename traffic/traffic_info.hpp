#pragma once

#include "traffic/speed_groups.hpp"

#include "indexer/mwm_set.hpp"

#include "std/cstdint.hpp"
#include "std/map.hpp"
#include "std/vector.hpp"

namespace traffic
{
// This class is responsible for providing the real-time
// information about road traffic for one mwm file.

class TrafficInfo
{
public:
  struct RoadSegmentId
  {
    // m_dir can be kForwardDirection or kReverseDirection.
    static int constexpr kForwardDirection = 0;
    static int constexpr kReverseDirection = 1;

    RoadSegmentId();

    RoadSegmentId(uint32_t fid, uint16_t idx, uint8_t dir);

    bool operator==(RoadSegmentId const & o) const
    {
      return m_fid == o.m_fid && m_idx == o.m_idx && m_dir == o.m_dir;
    }

    bool operator<(RoadSegmentId const & o) const
    {
      if (m_fid != o.m_fid)
        return m_fid < o.m_fid;
      if (m_idx != o.m_idx)
        return m_idx < o.m_idx;
      return m_dir < o.m_dir;
    }

    // The ordinal number of feature this segment belongs to.
    uint32_t m_fid;

    // The ordinal number of this segment in the list of
    // its feature's segments.
    uint16_t m_idx : 15;

    // The direction of the segment.
    uint8_t m_dir : 1;
  };

  // todo(@m) unordered_map?
  using Coloring = map<RoadSegmentId, SpeedGroup>;

  TrafficInfo() = default;

  TrafficInfo(MwmSet::MwmId const & mwmId);

  // Fetches the latest traffic data from the server and updates the coloring.
  // Construct the url by passing an MwmId.
  // *NOTE* This method must not be called on the UI thread.
  bool ReceiveTrafficData();

  // Returns the latest known speed group by a feature segment's id.
  SpeedGroup GetSpeedGroup(RoadSegmentId const & id) const;

  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }
  Coloring const & GetColoring() const { return m_coloring; }

  static void SerializeTrafficData(Coloring const & coloring, vector<uint8_t> & result);

  static void DeserializeTrafficData(vector<uint8_t> const & data, Coloring & coloring);

private:
  // The mapping from feature segments to speed groups (see speed_groups.hpp).
  Coloring m_coloring;
  MwmSet::MwmId m_mwmId;
};
}  // namespace traffic
