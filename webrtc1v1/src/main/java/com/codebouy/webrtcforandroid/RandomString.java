package com.codebouy.webrtcforandroid;

import java.util.Random;

/**
 * @Author: Codeboy 2685312867@qq.com
 * @Date: 2019-12-22 10:37
 */

public class RandomString {
    static final String str = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static final Random rnd = new Random();

    static public String length(Integer length) {
        StringBuilder sb = new StringBuilder(length);
        for (int i = 0; i < length; i++) {
            sb.append(str.charAt(rnd.nextInt(str.length())));
        }
        return sb.toString();
    }
}
