# Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


import tensorflow as tf

tf.compat.v1.enable_eager_execution()


def _round_up_tf(x, multiple):
    # Tf version of remainder = x % multiple
    remainder = tf.mod(x, multiple)
    # Tf version of return x if remainder == 0 else x + multiple, -remainder
    x_round = tf.cond(
        tf.equal(remainder, tf.zeros(tf.shape(remainder), dtype=tf.int32)),
        lambda: x,
        lambda: x + multiple,
        -remainder,
    )

    return x_round


def sequence_mask(lengths, r, expand=True):
    """Returns a 2-D or 3-D tensorflow sequence mask depending on the argument 'expand'"""
    max_len = tf.reduce_max(lengths)
    max_len = _round_up_tf(max_len, tf.convert_to_tensor(r))
    if expand:
        return tf.expand_dims(
            tf.sequence_mask(lengths, maxlen=max_len, dtype=tf.float32), axis=-1
        )
    return tf.sequence_mask(lengths, maxlen=max_len, dtype=tf.float32)


def MaskedSigmoidCrossEntropy(
    targets,
    outputs,
    targets_lengths,
    outputs_per_step,
    cross_entropy_pos_weight,
    mask=None,
):
    """Computes a masked SigmoidCrossEntropy with logits"""

    # [batch_size, time_dimension]
    # example:
    # sequence_mask([1, 3, 2], 5) = [[1., 0., 0., 0., 0.],
    #                                [1., 1., 1., 0., 0.],
    #                                [1., 1., 0., 0., 0.]]
    # Note the maxlen argument that ensures mask shape is compatible with r>1
    # This will by default mask the extra paddings caused by r>1
    if mask is None:
        mask = sequence_mask(targets_lengths, outputs_per_step, False)

    print("mask", mask)

    with tf.control_dependencies([tf.assert_equal(tf.shape(targets), tf.shape(mask))]):
        # Use a weighted sigmoid cross entropy to measure the <stop_token> loss. Set hparams.cross_entropy_pos_weight to 1
        # will have the same effect as  vanilla tf.nn.sigmoid_cross_entropy_with_logits.
        losses = tf.nn.weighted_cross_entropy_with_logits(
            targets=targets, logits=outputs, pos_weight=cross_entropy_pos_weight
        )
    print("losses", losses)
    with tf.control_dependencies([tf.assert_equal(tf.shape(mask), tf.shape(losses))]):
        masked_loss = losses * mask
    print("reduce_sum", tf.reduce_sum(masked_loss))
    print(
        "count_nonzero",
        tf.reduce_sum(tf.count_nonzero(tf.greater(tf.abs(masked_loss), 1e-6))),
    )
    return tf.reduce_sum(masked_loss) / tf.count_nonzero(
        tf.greater(tf.abs(masked_loss), 1e-6), dtype=tf.float32
    )


stop_token_prediction = tf.Variable(
    [
        [
            -19.2554722,
            -19.2172222,
            -19.7487106,
            -23.2418308,
            -23.0357475,
            -19.9446392,
            -27.0010834,
            -26.6425743,
            -28.5018578,
            -28.0486679,
            -27.7838459,
            -26.1214581,
            -27.1233,
            -26.907362,
            -22.8142586,
            -27.0453434,
            -26.7594566,
            -26.5863934,
            -26.3179436,
            -26.0884037,
            -25.2211628,
            -24.620985,
            -24.5142632,
            -22.1174355,
            -23.2725945,
            -23.1596012,
            -21.0120411,
            -23.2527103,
            -23.1696,
            -18.9072666,
            -23.2554893,
            -23.1731701,
            -18.4311829,
            -24.0848808,
            -24.1499138,
            -15.960947,
            -25.0092278,
            -24.9623737,
            -19.1189365,
            -26.3438778,
            -26.154995,
            -29.2581463,
            -25.3176689,
            -25.2528896,
            -28.2736454,
            -25.7639732,
            -25.7637558,
            -26.8779392,
            -24.7767124,
            -24.7134781,
            -22.3563709,
            -23.5989647,
            -23.6536312,
            -19.5809612,
            -23.2020531,
            -23.299015,
            -22.2237759,
            -23.5046,
            -23.6222115,
            -23.6591587,
            -23.600338,
            -23.6569653,
            -21.160244,
            -24.86063,
            -24.9315052,
            -22.037775,
            -24.9977646,
            -25.026741,
            -21.9423466,
            -23.6845512,
            -23.8408508,
            -19.2570915,
            -24.6328468,
            -24.8431606,
            -22.6878662,
            -23.3660145,
            -23.6384182,
            -21.5760536,
            -25.1034832,
            -25.348238,
            -23.8637543,
            -26.663229,
            -26.7145176,
            -25.336668,
            -27.2477226,
            -27.306406,
            -25.7301979,
            -28.7218399,
            -28.8692188,
            -26.3121185,
            -29.7668476,
            -29.9222355,
            -27.5231705,
            -28.9869289,
            -29.1099186,
            -26.162838,
            -26.6737137,
            -26.6174068,
            -23.7669525,
            -25.5801525,
            -25.382597,
            -24.8046799,
            -23.8789845,
            -23.697506,
            -24.2769432,
            -22.6485615,
            -22.6902733,
            -19.4312019,
            -25.5100574,
            -25.7145748,
            -24.8000069,
            -25.820364,
            -25.8850422,
            -24.9364567,
            -25.5461807,
            -25.6516953,
            -23.3298531,
            -26.0074,
            -26.2451801,
            -25.3116417,
            -27.9871769,
            -28.2245426,
            -27.5163536,
            -27.2617893,
            -27.499155,
            -26.7148914,
            -27.917696,
            -28.1484642,
            -26.3882656,
            -26.1964741,
            -26.4671707,
            -21.5169525,
            -25.7654495,
            -25.956953,
            -21.4978657,
            -25.4474277,
            -25.5888271,
            -21.8175259,
            -27.0709019,
            -27.2391357,
            -25.4488678,
            -26.6593227,
            -26.9093361,
            -24.8338413,
            -25.5061531,
            -25.8105183,
            -23.624918,
            -25.6026173,
            -25.8815746,
            -24.5043125,
            -25.9096336,
            -26.0886726,
            -25.2726841,
            -26.928133,
            -27.0977306,
            -26.1994705,
            -27.3337383,
            -27.4392586,
            -26.3144608,
            -27.7289352,
            -27.8782,
            -24.213871,
            -25.7142906,
            -25.8960228,
            -22.7322845,
            -25.6535549,
            -25.8980904,
            -24.5248547,
            -26.2106419,
            -26.4627228,
            -26.0687141,
            -26.5431461,
            -26.7009983,
            -25.7237949,
            -26.6101112,
            -26.6817436,
            -25.3532429,
            -26.7699566,
            -26.9373798,
            -26.9421253,
            -26.6608849,
            -26.9125614,
            -24.117918,
            -26.2250576,
            -26.5464878,
            -23.1006,
            -27.6363869,
            -27.8620872,
            -25.3283138,
            -29.9514732,
            -30.1839752,
            -26.477644,
            -29.3066959,
            -29.4690552,
            -23.5445633,
            -29.1421337,
            -29.2640915,
            -22.4593296,
            -28.1416054,
            -28.2274284,
            -22.18853,
            -26.733387,
            -26.7894478,
            -21.491787,
            -26.1531353,
            -26.1881218,
            -20.6900272,
            -24.7998199,
            -24.7371349,
            -19.1967335,
            -21.245182,
            -21.1941013,
            -15.3086586,
            -21.5838165,
            -21.5323563,
            -16.0762138,
            -21.4765892,
            -21.4428558,
            -15.7436924,
            -22.4633045,
            -22.4034424,
            -16.9948044,
            -22.7169952,
            -22.6739521,
            -17.8596611,
            -21.7312,
            -21.6699924,
            -14.5657434,
            -20.6252861,
            -20.6077271,
            -13.4298716,
            -22.6423855,
            -22.5940323,
            -18.3120213,
            -23.1737137,
            -23.0944881,
            -17.6568565,
            -22.7808514,
            -22.6918087,
            -16.7538891,
            -22.4906845,
            -22.4037457,
            -16.9209671,
            -23.021389,
            -22.9521198,
            -17.1286907,
            -22.9245148,
            -22.8395557,
            -16.294363,
            -21.9706516,
            -21.9045429,
            -15.5770559,
            -22.0162334,
            -21.9441986,
            -16.1142902,
            -23.2365379,
            -23.1556301,
            -17.2457199,
            -21.6759892,
            -21.6193886,
            -14.4782619,
            -21.6260395,
            -21.6478901,
            -10.49055,
        ]
    ],
    dtype=tf.float32,
)

stop_token_target = tf.Variable(
    [
        [
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            1,
        ]
    ],
    dtype=tf.float32,
)

targets_lengths = tf.Variable([264], dtype=tf.int32)

with tf.GradientTape() as tape:
    tape.watch(stop_token_prediction)
    stop_token_loss = MaskedSigmoidCrossEntropy(
        stop_token_target, stop_token_prediction, targets_lengths, 3, 1
    )


print(
    tf.nn.weighted_cross_entropy_with_logits(
        targets=[0.0], logits=[-14.4782], pos_weight=1
    )
)

print(stop_token_loss)

g = tape.gradient(stop_token_loss, stop_token_prediction)

# print(g)
