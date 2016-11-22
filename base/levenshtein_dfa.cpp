#include "base/levenshtein_dfa.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/queue.hpp"
#include "std/set.hpp"
#include "std/sstream.hpp"
#include "std/vector.hpp"

namespace strings
{
namespace
{
class TransitionTable
{
public:
  TransitionTable(UniString const & s, uint8_t maxErrors)
    : m_s(s), m_size(s.size()), m_maxErrors(maxErrors)
  {
  }

  void Move(LevenshteinDFA::State const & s, size_t prefixCharsToKeep, UniChar c,
            LevenshteinDFA::State & t)
  {
    t.Clear();
    for (auto const & p : s.m_positions)
      GetMoves(p, prefixCharsToKeep, c, t);
    t.Normalize();
  }

private:
  void GetMoves(LevenshteinDFA::Position const & p, size_t prefixCharsToKeep, UniChar c,
                LevenshteinDFA::State & t)
  {
    auto & ps = t.m_positions;

    if (p.m_offset < m_size && m_s[p.m_offset] == c)
    {
      ps.emplace_back(p.m_offset + 1, p.m_numErrors);
      return;
    }

    if (p.m_numErrors == m_maxErrors)
      return;

    if (p.m_offset < prefixCharsToKeep)
      return;

    ps.emplace_back(p.m_offset, p.m_numErrors + 1);

    if (p.m_offset == m_size)
      return;

    ps.emplace_back(p.m_offset + 1, p.m_numErrors + 1);

    size_t i;
    if (FindRelevant(p, c, i))
    {
      ASSERT_GREATER(i, 0, (i));
      ASSERT_LESS_OR_EQUAL(p.m_offset + i + 1, m_size, ());
      ps.emplace_back(p.m_offset + i + 1, p.m_numErrors + i);
    }
  }

  bool FindRelevant(LevenshteinDFA::Position const & p, UniChar c, size_t & i) const
  {
    size_t const limit =
        min(m_size - p.m_offset, static_cast<size_t>(m_maxErrors - p.m_numErrors) + 1);

    for (i = 0; i < limit; ++i)
    {
      if (m_s[p.m_offset + i] == c)
        return true;
    }
    return false;
  }

  UniString const & m_s;
  size_t const m_size;
  uint8_t const m_maxErrors;
};
}  // namespace

// LevenshteinDFA ----------------------------------------------------------------------------------
// static
size_t const LevenshteinDFA::kStartingState = 0;
size_t const LevenshteinDFA::kRejectingState = 1;

// LevenshteinDFA::Position ------------------------------------------------------------------------
LevenshteinDFA::Position::Position(size_t offset, uint8_t numErrors)
  : m_offset(offset), m_numErrors(numErrors)
{
}

bool LevenshteinDFA::Position::SubsumedBy(Position const & rhs) const
{
  if (m_numErrors <= rhs.m_numErrors)
    return false;

  size_t const u = m_offset < rhs.m_offset ? rhs.m_offset - m_offset : m_offset - rhs.m_offset;
  size_t const v = m_numErrors - rhs.m_numErrors;

  return u <= v;
}

bool LevenshteinDFA::Position::operator<(Position const & rhs) const
{
  if (m_offset != rhs.m_offset)
    return m_offset < rhs.m_offset;
  return m_numErrors < rhs.m_numErrors;
}

bool LevenshteinDFA::Position::operator==(Position const & rhs) const
{
  return m_offset == rhs.m_offset && m_numErrors == rhs.m_numErrors;
}

// LevenshteinDFA::State ---------------------------------------------------------------------------
// static
LevenshteinDFA::State LevenshteinDFA::State::MakeStart()
{
  State state;
  state.m_positions.emplace_back(0 /* offset */, 0 /* numErrors */);
  return state;
}

// static
LevenshteinDFA::State LevenshteinDFA::State::MakeRejecting()
{
  return State();
}

void LevenshteinDFA::State::Normalize()
{
  size_t j = m_positions.size();
  for (size_t i = 0; i < j; ++i)
  {
    auto const & cur = m_positions[i];

    auto it = find_if(m_positions.begin(), m_positions.begin() + j,
                      [&](Position const & rhs) { return cur.SubsumedBy(rhs); });
    if (it != m_positions.begin() + j)
    {
      ASSERT_GREATER(j, 0, ());
      --j;
      swap(m_positions[i], m_positions[j]);
    }
  }

  m_positions.erase(m_positions.begin() + j, m_positions.end());
  my::SortUnique(m_positions);
}

// LevenshteinDFA ----------------------------------------------------------------------------------
// static
LevenshteinDFA::LevenshteinDFA(UniString const & s, size_t prefixCharsToKeep, uint8_t maxErrors)
  : m_size(s.size()), m_maxErrors(maxErrors)
{
  m_alphabet.assign(s.begin(), s.end());
  my::SortUnique(m_alphabet);

  UniChar missed = 0;
  for (size_t i = 0; i < m_alphabet.size() && missed >= m_alphabet[i]; ++i)
  {
    if (missed == m_alphabet[i])
      ++missed;
  }
  m_alphabet.push_back(missed);

  queue<State> states;
  map<State, size_t> visited;

  auto pushState = [&states, &visited, this](State const & state, size_t id)
  {
    ASSERT_EQUAL(id, m_transitions.size(), ());
    ASSERT_EQUAL(visited.count(state), 0, (state, id));

    states.emplace(state);
    visited[state] = id;
    m_transitions.emplace_back(m_alphabet.size());
    m_accepting.push_back(false);
  };

  pushState(State::MakeStart(), kStartingState);
  pushState(State::MakeRejecting(), kRejectingState);

  TransitionTable table(s, maxErrors);

  while (!states.empty())
  {
    auto const curr = states.front();
    states.pop();
    ASSERT(IsValid(curr), (curr));

    ASSERT_GREATER(visited.count(curr), 0, (curr));
    auto const id = visited[curr];
    ASSERT_LESS(id, m_transitions.size(), ());

    if (IsAccepting(curr))
      m_accepting[id] = true;

    for (size_t i = 0; i < m_alphabet.size(); ++i)
    {
      State next;
      table.Move(curr, prefixCharsToKeep, m_alphabet[i], next);

      size_t nid;

      auto const it = visited.find(next);
      if (it == visited.end())
      {
        nid = visited.size();
        pushState(next, nid);
      }
      else
      {
        nid = it->second;
      }

      m_transitions[id][i] = nid;
    }
  }
}

LevenshteinDFA::LevenshteinDFA(string const & s, size_t prefixCharsToKeep, uint8_t maxErrors)
  : LevenshteinDFA(MakeUniString(s), prefixCharsToKeep, maxErrors)
{
}

LevenshteinDFA::LevenshteinDFA(UniString const & s, uint8_t maxErrors)
  : LevenshteinDFA(s, 0 /* prefixCharsToKeep */, maxErrors)
{
}

LevenshteinDFA::LevenshteinDFA(string const & s, uint8_t maxErrors)
  : LevenshteinDFA(s, 0 /* prefixCharsToKeep */, maxErrors)
{
}

bool LevenshteinDFA::IsValid(Position const & p) const
{
  return p.m_offset <= m_size && p.m_numErrors <= m_maxErrors;
}

bool LevenshteinDFA::IsValid(State const & s) const
{
  for (auto const & p : s.m_positions)
  {
    if (!IsValid(p))
      return false;
  }
  return true;
}

bool LevenshteinDFA::IsAccepting(Position const & p) const
{
  return m_size - p.m_offset <= m_maxErrors - p.m_numErrors;
}

bool LevenshteinDFA::IsAccepting(State const & s) const
{
  for (auto const & p : s.m_positions)
  {
    if (IsAccepting(p))
      return true;
  }
  return false;
}

size_t LevenshteinDFA::Move(size_t s, UniChar c) const
{
  ASSERT_GREATER(m_alphabet.size(), 0, ());
  ASSERT(is_sorted(m_alphabet.begin(), m_alphabet.end() - 1), ());

  size_t i;
  auto const it = lower_bound(m_alphabet.begin(), m_alphabet.end() - 1, c);
  if (it == m_alphabet.end() - 1 || *it != c)
    i = m_alphabet.size() - 1;
  else
    i = distance(m_alphabet.begin(), it);

  return m_transitions[s][i];
}

string DebugPrint(LevenshteinDFA::Position const & p)
{
  ostringstream os;
  os << "Position [" << p.m_offset << ", " << static_cast<uint32_t>(p.m_numErrors) << "]";
  return os.str();
}

string DebugPrint(LevenshteinDFA::State const & s)
{
  ostringstream os;
  os << "State [";
  for (size_t i = 0; i < s.m_positions.size(); ++i)
  {
    os << DebugPrint(s.m_positions[i]);
    if (i + 1 != s.m_positions.size())
      os << ", ";
  }
  return os.str();
}
}  // namespace strings
