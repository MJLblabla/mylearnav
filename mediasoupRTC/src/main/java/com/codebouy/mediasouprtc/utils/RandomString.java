package com.codebouy.mediasouprtc.utils;

import java.util.Random;



public class RandomString {
    static final String str = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static final String numstr = "0123456789";
    static final Random rnd = new Random();

    static public String length(Integer length) {
        StringBuilder sb = new StringBuilder(length);
        for (int i = 0; i < length; i++) {
            sb.append(str.charAt(rnd.nextInt(str.length())));
        }
        return sb.toString();
    }

    static public int numlength(Integer length) {
        StringBuilder sb = new StringBuilder(length);
        for (int i = 0; i < length; i++) {
            sb.append(numstr.charAt(rnd.nextInt(numstr.length())));
        }
        return Integer.parseInt(sb.toString());
    }
}
