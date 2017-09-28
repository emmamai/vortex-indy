Ability: Cripple
===============

Cripple is a [Necromancer](../classes/necromancer.md) ability which deals a percentage of its target's maximum health.

The command for cripple is:

    cripple

### Stats

| Stat                          |       |
| -------------                 | ---   |
| Cost                          | 25    |
| Cooldown                      | 2.0   |
| Range                         | 256   |
| Max damage                    | 500   |

The formula for determining damage dealt in percentage is:

    100 * ( 1 - ( 1 / ( 1 + 0.2 * SKILL_LEVEL ) ) )

This table shows the percentage damage dealt per level using this formula:

|   1    |   2    |   3    |   4    |   5    |   6    |   7    |   8    |   9    |   10   |...|   20   |...|   30   |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ |---| ------ |---| ------ |
| 16.66% | 28.57% | 37.5%  | 44.44% | 50.00% | 54.54% | 58.33% | 61.53% | 64.28% | 66.66% |...| 80.00% |...| 85.71% |

### Interactions

---