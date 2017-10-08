Ability: Resist
===============

Resist is a passive [Soldier](../classes/necromancer.md) and general ability which reduces the amount of damage taken.

### Stats

For PVM, the formula for determining damage taken in percentage is:

    1 / ( 1 + 0.1 * SKILL_LEVEL ) ) )

For PVP, the formula is:

    1 / ( 1 + 0.066 * SKILL_LEVEL ) ) )

This table shows the percentage damage taken per level in PVM:

|   1    |   2    |   3    |   4    |   5    |   6    |   7    |   8    |   9    |   10   |...|   20   |...|   30   |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ |---| ------ |---| ------ |
| 90.90% | 83.33% | 76.92% | 71.42% | 66.66% | 62.50% | 58.82% | 55.55% | 52.63% | 50.00% |...| 33.33% |...| 25.00% |

And in PVP:

|   1    |   2    |   3    |   4    |   5    |   6    |   7    |   8    |   9    |   10   |...|   20   |...|   30   |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ |---| ------ |---| ------ |
| 93.80% | 88.33% | 83.47% | 79.11% | 75.19% | 71.63% | 68.39% | 65.44% | 62.74% | 60.24% |...| 43.10% |...| 33.56% |

### Interactions

Resist does not stack with other resistance abilities - when calulating resistance, the strongest resistance is used
and the rest are ignored. This means that if a player hit by a bullet has 5 levels in Resist (66.66% taken) and also has
Bullet Resist (40% taken), the Bullet Resist will be used, and Resist ignored.