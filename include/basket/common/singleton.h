/*
 * Copyright (C) 2019  Hariharan Devarajan, Keith Bateman
 *
 * This file is part of Basket
 * 
 * Basket is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/*-------------------------------------------------------------------------
 *
 * Created: singleton.h
 * June 5 2018
 * Hariharan Devarajan <hdevarajan@hdfgroup.org>
 *
 * Purpose:Define singleton template for making a Singleton instances of
 * certain classes in Basket.
 *
 *-------------------------------------------------------------------------
 */

#ifndef INCLUDE_BASKET_COMMON_SINGLETON_H_
#define INCLUDE_BASKET_COMMON_SINGLETON_H_
#include <iostream>
#include <memory>
#include <utility>
/**
 * Make a class singleton when used with the class. format for class name T
 * Singleton<T>::GetInstance()
 * @tparam T
 */
 namespace basket{
template<typename T>
class Singleton {
  public:
    /**
     * Members of Singleton Class
     */
    /**
     * Uses unique pointer to build a static global instance of variable.
     * @tparam T
     * @return instance of T
     */
    template <typename... Args>
    static std::shared_ptr<T> GetInstance(Args... args) {
        if (instance == nullptr)
            instance = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
        return instance;
    }

    /**
     * Operators
     */
    Singleton& operator= (const Singleton) = delete; /* deleting = operatos*/
    /**
     * Constructor
     */
  public:
    Singleton(const Singleton&) = delete; /* deleting copy constructor. */

  protected:
    static std::shared_ptr<T> instance;
    Singleton() {} /* hidden default constructor. */
};

template<typename T>
std::shared_ptr<T> Singleton<T>::instance = nullptr;
 }
#endif  // INCLUDE_BASKET_COMMON_SINGLETON_H_
