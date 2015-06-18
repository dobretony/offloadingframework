package com.offloadingframework.offloadlibrary;

import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;

/**
 * Created by tony on 09.06.2015.
 */
public class RequestFactory {

    public abstract class OffloadRequest{

        private RequestType mType = null;
        protected char[] output = new char[100];
        protected int size;

        protected OffloadRequest(RequestType type){
            mType = type;
        }

        public abstract void buildRequest();

        public char[] getOutput(){
            return output;
        }

    }

    public class RMIRequest extends OffloadRequest{

        private HashMap<String, Object> parameters = null;
        private String methodName = null;

        protected RMIRequest(){
            super(RequestType.REQUEST_RMI);
            parameters = new HashMap<>();
        }

        public void buildRequest(){

            if(methodName == null)
                return;

            StringBuffer buffer = new StringBuffer();
            int size = 0;

            buffer.append(methodName + ";");

            for(String s : parameters.keySet()){
                buffer.append(s + ":" + parameters.get(s).toString() + ";");
            }

            size += buffer.length();

            this.size = size;
            this.output = new char[size];

            buffer.getChars(0, size, this.output, 0);
            Log.d("RequestFactory", "Created output for request: " + buffer);
        }

        public boolean setMethodName(String name){
            if(name.contains("./;:")){
                return false;
            }

            methodName = new String(name);
            return true;
        }

        public boolean addParameter(String name, Object value){

            if(name.contains("./;:")){
                Log.e("RMIRequest", "Parameter name contains illegal characters.");
                return false;
            }

            if(value instanceof Integer){
                Log.d("RMIRequest", "Adding parameter of type Integer " + value.toString());
            }else if(value instanceof String){
                Log.d("RMIRequest", "Adding parameter of type String " + value.toString());
            }else if(value instanceof Double){
                Log.d("RMIRequest", "Adding parameter of type Double " + value.toString());
            }else if(value instanceof Long){
                Log.d("RMIRequest", "Adding parameter of type Long " + value.toString());
            }else if(value instanceof Boolean){
                Log.d("RMIRequest", "Adding parameter of type Boolean " + value.toString());
            }else{
                Log.e("RMIRequest", "Unsupported parameter type. It can either be Integer, string, Double or Long.");
                return false;
            }

            parameters.put(name, value);
            return true;
        }

    }


}
